#include <stdio.h>
#include "platform.h"

#include "mqtt_client.h"
#include "mqtt_msg.h"
#include "transport.h"
#include "transport_tcp.h"
#include "transport_ssl.h"
#include "transport_ws.h"
#include "platform.h"
#include "mqtt_outbox.h"

#include <time.h>
#include <sys/time.h>
#include "apps/sntp/sntp.h"

/* using uri parser */
#include "http_parser.h"

#include "elm327.h"
#include "libGSM.h"

#define TASK_SEMAPHORE_WAIT 140000	// time to wait for mutex in miliseconds

static const char *TAG = "MQTT_CLIENT";

QueueHandle_t mqtt_mutex;

//volatile long long int epoch;

typedef struct mqtt_state
{
    mqtt_connect_info_t *connect_info;
    uint8_t *in_buffer;
    uint8_t *out_buffer;
    int in_buffer_length;
    int out_buffer_length;
    uint32_t message_length;
    uint32_t message_length_read;
    mqtt_message_t *outbound_message;
    mqtt_connection_t mqtt_connection;
    uint16_t pending_msg_id;
    int pending_msg_type;
    int pending_publish_qos;
    int pending_msg_count;
} mqtt_state_t;

typedef struct {
    mqtt_event_callback_t event_handle;
    int task_stack;
    int task_prio;
    char *uri;
    char *host;
    char *path;
    char *scheme;
    int port;
    bool auto_reconnect;
    void *user_context;
    int network_timeout_ms;
} mqtt_config_storage_t;

typedef enum {
    MQTT_STATE_ERROR = -1,
    MQTT_STATE_UNKNOWN = 0,
    MQTT_STATE_INIT,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_WAIT_TIMEOUT,
} mqtt_client_state_t;

struct esp_mqtt_client {
    transport_list_handle_t transport_list;
    transport_handle_t transport;
    mqtt_config_storage_t *config;
    mqtt_state_t  mqtt_state;
    mqtt_connect_info_t connect_info;
    mqtt_client_state_t state;
    long long keepalive_tick;
    long long reconnect_tick;
    int wait_timeout_ms;
    int auto_reconnect;
    esp_mqtt_event_t event;
    bool run;
    outbox_handle_t outbox;
    EventGroupHandle_t status_bits;
    QueueHandle_t queue;
};

const static int STOPPED_BIT = BIT0;

static esp_err_t esp_mqtt_dispatch_event(esp_mqtt_client_handle_t client);
static esp_err_t esp_mqtt_set_config(esp_mqtt_client_handle_t client, const esp_mqtt_client_config_t *config);
static esp_err_t esp_mqtt_destroy_config(esp_mqtt_client_handle_t client);
static esp_err_t esp_mqtt_connect(esp_mqtt_client_handle_t client, int timeout_ms);
static esp_err_t esp_mqtt_abort_connection(esp_mqtt_client_handle_t client);
static esp_err_t esp_mqtt_client_ping(esp_mqtt_client_handle_t client);
static char *create_string(const char *ptr, int len);

char* getClientState(mqtt_client_state_t estado){
    if (estado == MQTT_STATE_ERROR){
        return "MQTT_STATE_ERROR";
    } else
    if (estado == MQTT_STATE_UNKNOWN){
        return "MQTT_STATE_UNKNOWN";
    } else
    if (estado == MQTT_STATE_INIT){
        return "MQTT_STATE_INIT";
    } else
    if (estado == MQTT_STATE_CONNECTED){
        return "MQTT_STATE_CONNECTED";
    } else
    if (estado == MQTT_STATE_WAIT_TIMEOUT){
        return "MQTT_STATE_WAIT_TIMEOUT";
    } else {
        return "None";
    }
}

static esp_err_t esp_mqtt_set_config(esp_mqtt_client_handle_t client, const esp_mqtt_client_config_t *config)
{
    //Copy user configurations to client context
    esp_err_t err = ESP_OK;
    mqtt_config_storage_t *cfg = calloc(1, sizeof(mqtt_config_storage_t));
    ESP_MEM_CHECK(TAG, cfg, return ESP_ERR_NO_MEM);

    client->config = cfg;

    cfg->task_prio = config->task_prio;
    if (cfg->task_prio <= 0) {
        cfg->task_prio = MQTT_TASK_PRIORITY;
    }

    cfg->task_stack = config->task_stack;
    if (cfg->task_stack == 0) {
        cfg->task_stack = MQTT_TASK_STACK;
    }
    err = ESP_ERR_NO_MEM;
    if (config->host) {
        cfg->host = strdup(config->host);
        ESP_MEM_CHECK(TAG, cfg->host, goto _mqtt_set_config_failed);
    }
    cfg->port = config->port;

    if (config->username) {
        client->connect_info.username = strdup(config->username);
        ESP_MEM_CHECK(TAG, client->connect_info.username, goto _mqtt_set_config_failed);
    }

    if (config->password) {
        client->connect_info.password = strdup(config->password);
        ESP_MEM_CHECK(TAG, client->connect_info.password, goto _mqtt_set_config_failed);
    }

    if (config->client_id) {
        client->connect_info.client_id = strdup(config->client_id);
    } else {
        client->connect_info.client_id = platform_create_id_string();
    }
    ESP_MEM_CHECK(TAG, client->connect_info.client_id, goto _mqtt_set_config_failed);
    ESP_LOGD(TAG, "MQTT client_id=%s", client->connect_info.client_id);
    ESP_LOGI(TAG, "MQTT client_id=%s", client->connect_info.client_id);

    if (config->uri) {
        cfg->uri = strdup(config->uri);
        ESP_MEM_CHECK(TAG, cfg->uri, goto _mqtt_set_config_failed);
    }

    if (config->lwt_topic) {
        client->connect_info.will_topic = strdup(config->lwt_topic);
        ESP_MEM_CHECK(TAG, client->connect_info.will_topic, goto _mqtt_set_config_failed);
    }

    if (config->lwt_msg_len) {
        client->connect_info.will_message = malloc(config->lwt_msg_len);
        ESP_MEM_CHECK(TAG, client->connect_info.will_message, goto _mqtt_set_config_failed);
        memcpy(client->connect_info.will_message, config->lwt_msg, config->lwt_msg_len);
        client->connect_info.will_length = config->lwt_msg_len;
    } else if (config->lwt_msg) {
        client->connect_info.will_message = strdup(config->lwt_msg);
        ESP_MEM_CHECK(TAG, client->connect_info.will_message, goto _mqtt_set_config_failed);
        client->connect_info.will_length = strlen(config->lwt_msg);
    }

    client->connect_info.will_qos = config->lwt_qos;
    client->connect_info.will_retain = config->lwt_retain;

    client->connect_info.clean_session = 1;
    if (config->disable_clean_session) {
        client->connect_info.clean_session = false;
    }
    client->connect_info.keepalive = config->keepalive;
    if (client->connect_info.keepalive == 0) {
        client->connect_info.keepalive = MQTT_KEEPALIVE_TICK;
    }
    cfg->network_timeout_ms = MQTT_NETWORK_TIMEOUT_MS;
    cfg->user_context = config->user_context;
    cfg->event_handle = config->event_handle;
    cfg->auto_reconnect = true;
    if (config->disable_auto_reconnect) {
        cfg->auto_reconnect = false;
    }


    return err;
_mqtt_set_config_failed:
    ESP_LOGE(TAG, "Fallida configuracion del MQTT en esp_mqtt_set_config");                           
    esp_mqtt_destroy_config(client);
    return err;
}

static esp_err_t esp_mqtt_destroy_config(esp_mqtt_client_handle_t client)
{
    mqtt_config_storage_t *cfg = client->config;
    free(cfg->host);
    free(cfg->uri);
    free(cfg->path);
    free(cfg->scheme);
    free(client->connect_info.will_topic);
    free(client->connect_info.will_message);
    free(client->connect_info.client_id);
    free(client->connect_info.username);
    free(client->connect_info.password);
    free(client->config);
    return ESP_OK;
}

static esp_err_t esp_mqtt_connect(esp_mqtt_client_handle_t client, int timeout_ms)
{
    int write_len, read_len, connect_rsp_code;
    mqtt_msg_init(&client->mqtt_state.mqtt_connection,
                  client->mqtt_state.out_buffer,
                  client->mqtt_state.out_buffer_length);
    client->mqtt_state.outbound_message = mqtt_msg_connect(&client->mqtt_state.mqtt_connection,
                                          client->mqtt_state.connect_info);
    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data,
                                        client->mqtt_state.outbound_message->length);
    ESP_LOGI(TAG, "Sending MQTT CONNECT message, type: %d, id: %04X",
             client->mqtt_state.pending_msg_type,
             client->mqtt_state.pending_msg_id);

    write_len = transport_write(client->transport,
                                (char *)client->mqtt_state.outbound_message->data,
                                client->mqtt_state.outbound_message->length,
                                client->config->network_timeout_ms);
    ESP_LOGI(TAG, "Mensaje %s - tamanno %d - escrito %d", (char *)client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length, write_len);                           
    if (write_len < 0) {
        ESP_LOGE(TAG, "Writing failed, errno= %d", errno);
        return ESP_FAIL;
    }
    read_len = transport_read(client->transport,
                              (char *)client->mqtt_state.in_buffer,
                              client->mqtt_state.outbound_message->length,
                              client->config->network_timeout_ms);
    if (read_len < 0) {
        ESP_LOGE(TAG, "Error network response");
        return ESP_FAIL;
    }

    if (mqtt_get_type(client->mqtt_state.in_buffer) != MQTT_MSG_TYPE_CONNACK) {
        ESP_LOGE(TAG, "Invalid MSG_TYPE response: %d, read_len: %d", mqtt_get_type(client->mqtt_state.in_buffer), read_len);
        return ESP_FAIL;
    }
    connect_rsp_code = mqtt_get_connect_return_code(client->mqtt_state.in_buffer);
    switch (connect_rsp_code) {
        case CONNECTION_ACCEPTED:
            ESP_LOGD(TAG, "Connected");
            return ESP_OK;
        case CONNECTION_REFUSE_PROTOCOL:
            ESP_LOGW(TAG, "Connection refused, bad protocol");
            return ESP_FAIL;
        case CONNECTION_REFUSE_SERVER_UNAVAILABLE:
            ESP_LOGW(TAG, "Connection refused, server unavailable");
            return ESP_FAIL;
        case CONNECTION_REFUSE_BAD_USERNAME:
            ESP_LOGW(TAG, "Connection refused, bad username or password");
            return ESP_FAIL;
        case CONNECTION_REFUSE_NOT_AUTHORIZED:
            ESP_LOGW(TAG, "Connection refused, not authorized");
            return ESP_FAIL;
        default:
            ESP_LOGW(TAG, "Connection refused, Unknow reason");
            return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t esp_mqtt_abort_connection(esp_mqtt_client_handle_t client)
{
    transport_close(client->transport);
    client->wait_timeout_ms = MQTT_RECONNECT_TIMEOUT_MS;
    client->reconnect_tick = platform_tick_get_ms();
    client->state = MQTT_STATE_WAIT_TIMEOUT;
    ESP_LOGI(TAG, "Reconnect after %d ms", client->wait_timeout_ms);
    client->event.event_id = MQTT_EVENT_DISCONNECTED;
    esp_mqtt_dispatch_event(client);
    return ESP_OK;
}

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config, QueueHandle_t queue)
{
    //ESP_LOGE(TAG, "Creando al cliente");
    esp_mqtt_client_handle_t client = calloc(1, sizeof(struct esp_mqtt_client));
    ESP_MEM_CHECK(TAG, client, return NULL);

    esp_mqtt_set_config(client, config);
    //ESP_LOGE(TAG, "Aplicada configuracion al cliente - Estado del cliente %s", getClientState(client->state));

    client->transport_list = transport_list_init();
    ESP_MEM_CHECK(TAG, client->transport_list, goto _mqtt_init_failed);
    //ESP_LOGE(TAG, "transport_list_init - Estado del cliente %s", getClientState(client->state));

    transport_handle_t tcp = transport_tcp_init();
    ESP_MEM_CHECK(TAG, tcp, goto _mqtt_init_failed);
    //ESP_LOGE(TAG, "transport_tcp_init - Estado del cliente %s", getClientState(client->state));
    transport_set_default_port(tcp, MQTT_TCP_DEFAULT_PORT);
    transport_list_add(client->transport_list, tcp, "mqtt");
    if (config->transport == MQTT_TRANSPORT_OVER_TCP) {
        client->config->scheme = create_string("mqtt", 4);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }
    client->queue = queue;
#if MQTT_ENABLE_WS
    transport_handle_t ws = transport_ws_init(tcp);
    ESP_MEM_CHECK(TAG, ws, goto _mqtt_init_failed);
    transport_set_default_port(ws, MQTT_WS_DEFAULT_PORT);
    transport_list_add(client->transport_list, ws, "ws");
    if (config->transport == MQTT_TRANSPORT_OVER_WS) {
        client->config->scheme = create_string("ws", 2);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }
#endif

#if MQTT_ENABLE_SSL
    transport_handle_t ssl = transport_ssl_init();
    ESP_MEM_CHECK(TAG, ssl, goto _mqtt_init_failed);
    transport_set_default_port(ssl, MQTT_SSL_DEFAULT_PORT);
    if (config->cert_pem) {
        transport_ssl_set_cert_data(ssl, config->cert_pem, strlen(config->cert_pem));
    }
    transport_list_add(client->transport_list, ssl, "mqtts");
    if (config->transport == MQTT_TRANSPORT_OVER_SSL) {
        client->config->scheme = create_string("mqtts", 5);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }
#endif

#if MQTT_ENABLE_WSS
    transport_handle_t wss = transport_ws_init(ssl);
    ESP_MEM_CHECK(TAG, wss, goto _mqtt_init_failed);
    transport_set_default_port(wss, MQTT_WSS_DEFAULT_PORT);
    transport_list_add(client->transport_list, wss, "wss");
    if (config->transport == MQTT_TRANSPORT_OVER_WSS) {
        client->config->scheme = create_string("wss", 3);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }
#endif
    //ESP_LOGE(TAG, "Iniciado el tcp");
    if (client->config->uri) {
        if (esp_mqtt_client_set_uri(client, client->config->uri) != ESP_OK) {
            goto _mqtt_init_failed;
        }
    }
    //ESP_LOGE(TAG, "seteado el uri - Estado del cliente %s", getClientState(client->state));

    if (client->config->scheme == NULL) {
        client->config->scheme = create_string("mqtt", 4);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }
    //ESP_LOGE(TAG, "seteado el scheme - Estado del cliente %s", getClientState(client->state));

    client->keepalive_tick = platform_tick_get_ms();
    client->reconnect_tick = platform_tick_get_ms();
    //ESP_LOGE(TAG, "seteados los *_tick - Estado del cliente %s", getClientState(client->state));

    int buffer_size = config->buffer_size;
    if (buffer_size <= 0) {
        buffer_size = MQTT_BUFFER_SIZE_BYTE;
    }

    client->mqtt_state.in_buffer = (uint8_t *)malloc(buffer_size);
    ESP_MEM_CHECK(TAG, client->mqtt_state.in_buffer, goto _mqtt_init_failed);
    //ESP_LOGE(TAG, "seteado el in_buffer - Estado del cliente %s", getClientState(client->state));
    client->mqtt_state.in_buffer_length = buffer_size;
    client->mqtt_state.out_buffer = (uint8_t *)malloc(buffer_size);
    ESP_MEM_CHECK(TAG, client->mqtt_state.out_buffer, goto _mqtt_init_failed);
    //ESP_LOGE(TAG, "seteado el out_buffer - Estado del cliente %s", getClientState(client->state));

    client->mqtt_state.out_buffer_length = buffer_size;
    client->mqtt_state.connect_info = &client->connect_info;
    //ESP_LOGE(TAG, "seteado el connect_info - Estado del cliente %s", getClientState(client->state));
    client->outbox = outbox_init();
    ESP_MEM_CHECK(TAG, client->outbox, goto _mqtt_init_failed);
    //ESP_LOGE(TAG, "ejecutado el outbox_init - Estado del cliente %s", getClientState(client->state));
    client->status_bits = xEventGroupCreate();
    ESP_MEM_CHECK(TAG, client->status_bits, goto _mqtt_init_failed);
    //ESP_LOGE(TAG, "ejecutado el xEventGroupCreate - Estado del cliente %s", getClientState(client->state));
    return client;
_mqtt_init_failed:
    //ESP_LOGE(TAG, "ejecutado el _mqtt_init_failed - Estado del cliente %s", getClientState(client->state));
    esp_mqtt_client_destroy(client);
    //ESP_LOGE(TAG, "Destruido el cliente");
    return NULL;
}

esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client)
{
    esp_mqtt_client_stop(client);
    esp_mqtt_destroy_config(client);
    transport_list_destroy(client->transport_list);
    outbox_destroy(client->outbox);
    vEventGroupDelete(client->status_bits);
    free(client->mqtt_state.in_buffer);
    free(client->mqtt_state.out_buffer);
    free(client);
    return ESP_OK;
}

static char *create_string(const char *ptr, int len)
{
    char *ret;
    if (len <= 0) {
        return NULL;
    }
    ret = calloc(1, len + 1);
    ESP_MEM_CHECK(TAG, ret, return NULL);
    memcpy(ret, ptr, len);
    return ret;
}

esp_err_t esp_mqtt_client_set_uri(esp_mqtt_client_handle_t client, const char *uri)
{
    struct http_parser_url puri;
    http_parser_url_init(&puri);
    int parser_status = http_parser_parse_url(uri, strlen(uri), 0, &puri);
    if (parser_status != 0) {
        ESP_LOGE(TAG, "Error parse uri = %s", uri);
        return ESP_FAIL;
    }

    if (client->config->scheme == NULL) {
        client->config->scheme = create_string(uri + puri.field_data[UF_SCHEMA].off, puri.field_data[UF_SCHEMA].len);
    }

    if (client->config->host == NULL) {
        client->config->host = create_string(uri + puri.field_data[UF_HOST].off, puri.field_data[UF_HOST].len);
    }

    if (client->config->path == NULL) {
        client->config->path = create_string(uri + puri.field_data[UF_PATH].off, puri.field_data[UF_PATH].len);
    }
    if (client->config->path) {
        transport_handle_t trans = transport_list_get_transport(client->transport_list, "ws");
        if (trans) {
            transport_ws_set_path(trans, client->config->path);
        }
        trans = transport_list_get_transport(client->transport_list, "wss");
        if (trans) {
            transport_ws_set_path(trans, client->config->path);
        }
    }

    if (puri.field_data[UF_PORT].len) {
        client->config->port = strtol((const char*)(uri + puri.field_data[UF_PORT].off), NULL, 10);
    }

    char *user_info = create_string(uri + puri.field_data[UF_USERINFO].off, puri.field_data[UF_USERINFO].len);
    if (user_info) {
        char *pass = strchr(user_info, ':');
        if (pass) {
            pass[0] = 0; //terminal username
            pass ++;
            client->connect_info.password = strdup(pass);
        }
        client->connect_info.username = strdup(user_info);

        free(user_info);
    }

    return ESP_OK;
}

static esp_err_t mqtt_write_data(esp_mqtt_client_handle_t client)
{
    int write_len = transport_write(client->transport,
                                    (char *)client->mqtt_state.outbound_message->data,
                                    client->mqtt_state.outbound_message->length,
                                    client->config->network_timeout_ms);
    // client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    if (write_len <= 0) {
        ESP_LOGE(TAG, "Error write data or timeout, written len = %d", write_len);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t esp_mqtt_dispatch_event(esp_mqtt_client_handle_t client)
{
    client->event.msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);
    client->event.user_context = client->config->user_context;
    client->event.client = client;

    if (client->config->event_handle) {
        return client->config->event_handle(&client->event);
    }
    return ESP_FAIL;
}



static void deliver_publish(esp_mqtt_client_handle_t client, uint8_t *message, int length)
{
    const char *mqtt_topic, *mqtt_data;
    uint32_t mqtt_topic_length, mqtt_data_length;
    uint32_t mqtt_len, mqtt_offset = 0, total_mqtt_len = 0;
    int len_read;

    do
    {
        if (total_mqtt_len == 0) {
            mqtt_topic_length = length;
            mqtt_topic = mqtt_get_publish_topic(message, &mqtt_topic_length);
            mqtt_data_length = length;
            mqtt_data = mqtt_get_publish_data(message, &mqtt_data_length);
            total_mqtt_len = client->mqtt_state.message_length - client->mqtt_state.message_length_read + mqtt_data_length;
            mqtt_len = mqtt_data_length;
        } else {
            mqtt_len = len_read;
            mqtt_data = (const char*)client->mqtt_state.in_buffer;
        }

        ESP_LOGD(TAG, "Get data len= %d, topic len=%d", mqtt_len, mqtt_topic_length);
        client->event.event_id = MQTT_EVENT_DATA;
        client->event.data = (char *)mqtt_data;
        client->event.data_len = mqtt_len;
        client->event.total_data_len = total_mqtt_len;
        client->event.current_data_offset = mqtt_offset;
        client->event.topic = (char *)mqtt_topic;
        client->event.topic_len = mqtt_topic_length;
        esp_mqtt_dispatch_event(client);

        mqtt_offset += mqtt_len;
        if (client->mqtt_state.message_length_read >= client->mqtt_state.message_length) {
            break;
        }

        len_read = transport_read(client->transport,
                                  (char *)client->mqtt_state.in_buffer,
                                  client->mqtt_state.message_length - client->mqtt_state.message_length_read > client->mqtt_state.in_buffer_length ?
                                  client->mqtt_state.in_buffer_length : client->mqtt_state.message_length - client->mqtt_state.message_length_read,
                                  client->config->network_timeout_ms);
        if (len_read <= 0) {
            ESP_LOGE(TAG, "Read error or timeout: %d", errno);
            break;
        }
        client->mqtt_state.message_length_read += len_read;
    } while (1);


}

static bool is_valid_mqtt_msg(esp_mqtt_client_handle_t client, int msg_type, int msg_id)
{
    ESP_LOGD(TAG, "pending_id=%d, pending_msg_count = %d", client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_count);
    if (client->mqtt_state.pending_msg_count == 0) {
        return false;
    }
    if (outbox_delete(client->outbox, msg_id, msg_type) == ESP_OK) {
        client->mqtt_state.pending_msg_count --;
        return true;
    }
    if (client->mqtt_state.pending_msg_type == msg_type && client->mqtt_state.pending_msg_id == msg_id) {
        client->mqtt_state.pending_msg_count --;
        return true;
    }

    return false;
}

static void mqtt_enqueue(esp_mqtt_client_handle_t client)
{
    ESP_LOGD(TAG, "mqtt_enqueue id: %d, type=%d successful",
             client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    //lock mutex
    if (client->mqtt_state.pending_msg_count > 0) {
        //Copy to queue buffer
        outbox_enqueue(client->outbox,
                       client->mqtt_state.outbound_message->data,
                       client->mqtt_state.outbound_message->length,
                       client->mqtt_state.pending_msg_id,
                       client->mqtt_state.pending_msg_type,
                       platform_tick_get_ms());
    }
    //unlock
}

static esp_err_t mqtt_process_receive(esp_mqtt_client_handle_t client)
{
    int read_len;
    uint8_t msg_type;
    uint8_t msg_qos;
    uint16_t msg_id;

    read_len = transport_read(client->transport, (char *)client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length, 1000);

    if (read_len < 0) {
        ESP_LOGE(TAG, "Read error or end of stream");
        return ESP_FAIL;
    }

    if (read_len == 0) {
        return ESP_OK;
    }

    msg_type = mqtt_get_type(client->mqtt_state.in_buffer);
    msg_qos = mqtt_get_qos(client->mqtt_state.in_buffer);
    msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);

    ESP_LOGD(TAG, "msg_type=%d, msg_id=%d", msg_type, msg_id);
    switch (msg_type)
    {
        case MQTT_MSG_TYPE_SUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_SUBSCRIBE, msg_id)) {
                ESP_LOGD(TAG, "Subscribe successful");
                client->event.event_id = MQTT_EVENT_SUBSCRIBED;
                esp_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_UNSUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_UNSUBSCRIBE, msg_id)) {
                ESP_LOGD(TAG, "UnSubscribe successful");
                client->event.event_id = MQTT_EVENT_UNSUBSCRIBED;
                esp_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_PUBLISH:
            if (msg_qos == 1) {
                client->mqtt_state.outbound_message = mqtt_msg_puback(&client->mqtt_state.mqtt_connection, msg_id);
            }
            else if (msg_qos == 2) {
                client->mqtt_state.outbound_message = mqtt_msg_pubrec(&client->mqtt_state.mqtt_connection, msg_id);
            }

            if (msg_qos == 1 || msg_qos == 2) {
                ESP_LOGD(TAG, "Queue response QoS: %d", msg_qos);

                if (mqtt_write_data(client) != ESP_OK) {
                    ESP_LOGE(TAG, "Error write qos msg repsonse, qos = %d", msg_qos);
                    // TODO: Shoule reconnect?
                    // return ESP_FAIL;
                }
            }
            client->mqtt_state.message_length_read = read_len;
            client->mqtt_state.message_length = mqtt_get_total_length(client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
            ESP_LOGI(TAG, "deliver_publish, message_length_read=%d, message_length=%d", read_len, client->mqtt_state.message_length);

            deliver_publish(client, client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
            break;
        case MQTT_MSG_TYPE_PUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_PUBLISH, msg_id)) {
                ESP_LOGD(TAG, "received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish");
                client->event.event_id = MQTT_EVENT_PUBLISHED;
                esp_mqtt_dispatch_event(client);
            }

            break;
        case MQTT_MSG_TYPE_PUBREC:
            ESP_LOGD(TAG, "received MQTT_MSG_TYPE_PUBREC");
            client->mqtt_state.outbound_message = mqtt_msg_pubrel(&client->mqtt_state.mqtt_connection, msg_id);
            mqtt_write_data(client);
            break;
        case MQTT_MSG_TYPE_PUBREL:
            ESP_LOGD(TAG, "received MQTT_MSG_TYPE_PUBREL");
            client->mqtt_state.outbound_message = mqtt_msg_pubcomp(&client->mqtt_state.mqtt_connection, msg_id);
            mqtt_write_data(client);

            break;
        case MQTT_MSG_TYPE_PUBCOMP:
            ESP_LOGD(TAG, "received MQTT_MSG_TYPE_PUBCOMP");
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_PUBREL, msg_id)) {
                ESP_LOGD(TAG, "Receive MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish");
                client->event.event_id = MQTT_EVENT_PUBLISHED;
                esp_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_PINGREQ:
            client->mqtt_state.outbound_message = mqtt_msg_pingresp(&client->mqtt_state.mqtt_connection);
            mqtt_write_data(client);
            break;
        case MQTT_MSG_TYPE_PINGRESP:
            ESP_LOGD(TAG, "MQTT_MSG_TYPE_PINGRESP");
            // Ignore
            break;
    }

    return ESP_OK;
}

static message_MQTT* msgMQTT(message_MQTT* msg, elm327_data_t pxRxedMessage, int num_intento, long long int epoch)
{
    char *strIntento = (char *)pvPortMalloc(10);
    char *strftime_buf = (char *)pvPortMalloc(128);
    char *tmp = (char *)pvPortMalloc(32);
    char *tmp2 = (char *)pvPortMalloc(32);
    char *tmp3 = (char *)pvPortMalloc(32);
    char *vin = (char *)pvPortMalloc(18);
    char *lat = (char *)pvPortMalloc(13);
    char *plat;
    char *lon = (char *)pvPortMalloc(13);
    char *plon;
    char *tim = (char *)pvPortMalloc(13);
    double coord = 0;
    double aux1 = 0;
    time_t fecha;
    char dato[2];
    struct tm tiempo;
    int size = 0;
    int i = 0;
    int j = 0;
    long long int epoch_l = 0;
    size = sizeof(pxRxedMessage.VIN)/sizeof(pxRxedMessage.VIN[0]);
    j = 0;
    for (i = 0; i < size ; i++){
        if (strlen((char *)&pxRxedMessage.VIN[i]) != 0){
            vin[j++] = (char)pxRxedMessage.VIN[i];
        }
    }
    vin[j] = '\0';
    msg->GPS = true;
    strcpy(msg->msg, "Camioneta_ESP32,");
    strcpy(msg->msgGPS, "Camioneta_ESP32_GPS,");
    sprintf(tmp, "VIN=\"%s\" ", vin);
    strcat(msg->msg, tmp);
    strcat(msg->msgGPS, tmp);
    if ((FUEL_FIELD & pxRxedMessage.fields) != 0){
        aux1 = atof((char *)&pxRxedMessage.fuel)/2.55;
        sprintf(tmp, "combustible=%f,", aux1);
        strcat(msg->msg, tmp);
        strcat(msg->msgGPS, tmp);
    }
    if ((SPEED_FIELD & pxRxedMessage.fields) != 0){
        aux1 = atof((char *)&pxRxedMessage.speed);
        sprintf(tmp, "velocidad=%f,", aux1);
        strcat(msg->msg, tmp);
        strcat(msg->msgGPS, tmp);
    }
    if ((TEMP_FIELD & pxRxedMessage.fields) != 0){
        aux1 = atof((char *)&pxRxedMessage.temp) - 40;
        sprintf(tmp, "temperatura=%f,", aux1);
        strcat(msg->msg, tmp);
        strcat(msg->msgGPS, tmp);
    }
    if ((LONG_FIELD & pxRxedMessage.fields) != 0){
        size = 11; //sizeof(pxRxedMessage.LONG)/sizeof(pxRxedMessage.LONG[0]);
        j = 0;
        for (i = 0; i < size ; i++){
            lon[j++] = (char)pxRxedMessage.LONG[i];
        }
        lon[j] = '\0';
        plon = strtok(lon,",");
        if (plon != NULL){
            sscanf( plon, "%3s%2s%2s", tmp, tmp2, tmp3 );
            coord = atof(tmp);
            sprintf(lon,"%i.%i", atoi(tmp2), atoi(tmp3));
            coord += atof(lon)/60;
        } else {
            msg->GPS = false;
        }
        plon = strtok(NULL, ",");
        if (plon != NULL){
            if (strcmp(plon,"W") == 0){
                strcat(msg->msgGPS, "longitud=-");
            } else {
                strcat(msg->msgGPS, "longitud=");
            }
            sprintf(lon,"%f",coord);
            strcat(msg->msgGPS, lon);
            strcat(msg->msgGPS, ",");
        } else {
           msg->GPS = false;
        }
    } else {
        msg->GPS = false;
    }
    if ((LAT_FIELD & pxRxedMessage.fields) != 0){
        size = 10; //sizeof(pxRxedMessage.LAT)/sizeof(pxRxedMessage.LAT[0]);
        j = 0;
        for (i = 0; i < size ; i++){
            lat[j++] = (char)pxRxedMessage.LAT[i];
        }
        lat[j] = '\0';
        plat = strtok(lat,",");
        if (plat != NULL){
            sscanf( plat, "%2s%2s%2s", tmp, tmp2, tmp3 );
            coord = atof(tmp);
            sprintf(lat,"%i.%i", atoi(tmp2), atoi(tmp3));
            coord += atof(lat)/60;
        } else {
            msg->GPS = false;
        }
        plat = strtok(NULL, ",");
        if (plat != NULL){
            if (strcmp(plat,"S") == 0){
                strcat(msg->msgGPS, "latitud=-");
            } else {
                strcat(msg->msgGPS, "latitud=");
            }
            sprintf(lat,"%f",coord);
            strcat(msg->msgGPS, lat);
            strcat(msg->msgGPS, ",");
        } else {
           msg->GPS = false;
        }
    } else {
        msg->GPS = false;
    }
    if ((TIME_FIELD & pxRxedMessage.fields) != 0){
        size = sizeof(pxRxedMessage.TIME)/sizeof(pxRxedMessage.TIME[0]);
        j = 0;
        for (i = 0; i < size ; i++){
            tim[j++] = (char)pxRxedMessage.TIME[i];
        }
        tim[j] = '\0';
        dato[0] = tim[0];
        dato[1] = tim[1];
        tiempo.tm_hour = atoi(dato);

        dato[0] = tim[2];
        dato[1] = tim[3];
        tiempo.tm_min = atoi(dato);

        dato[0] = tim[4];
        dato[1] = tim[5];
        tiempo.tm_sec = atoi(dato);

        dato[0] = tim[6];
        dato[1] = tim[7];
        tiempo.tm_mday = atoi(dato);

        dato[0] = tim[8];
        dato[1] = tim[9];
        tiempo.tm_mon = atoi(dato) - 1;

        dato[0] = tim[10];
        dato[1] = tim[11];
        tiempo.tm_year = atoi(dato) + 100;

        fecha = mktime(&tiempo);        
        epoch_l = (unsigned long long int)fecha;
        
        sprintf(tmp,"tiempo=%Li", epoch_l);
        strcat(msg->msgGPS,tmp);
        strcat(msg->msgGPS,"000000000,");
    } else {
        msg->GPS = false;
    }
    sprintf(tmp, "VIN=\"%s\",", vin);
    strcat(msg->msg, tmp);
    strcat(msg->msgGPS, tmp);
    strcat(msg->msg,"intento=");
    strcat(msg->msgGPS,"intento=");
    itoa(num_intento, strIntento, 10);
    strcat(msg->msg, strIntento);
    strcat(msg->msgGPS, strIntento);
    strcat(msg->msg, " ");
    strcat(msg->msgGPS, " ");
    sprintf(strftime_buf, "%Li", epoch );
    strcat(msg->msg, strftime_buf);
    strcat(msg->msg, "000000000");
    if (msg->GPS){
        sprintf(strftime_buf, "%Li", epoch_l );
        strcat(msg->msgGPS, strftime_buf);
        strcat(msg->msgGPS, "000000000");
    }
    vPortFree(strIntento);
    vPortFree(strftime_buf);
    vPortFree(tmp);
    vPortFree(tmp2);
    vPortFree(tmp3);
    vPortFree(vin);
    vPortFree(lat);
    vPortFree(lon);
    vPortFree(tim);
    return msg;
}

static void esp_mqtt_task(void *pv)
{
    ESP_LOGI(TAG, "Iniciando esp_mqtt_task");
    message_MQTT mensaje;
    int num_intento = 0;
    int msg_id = 0;
    int msg_id2 = 0;
    time_t now = 0;
    elm327_data_t pxRxedMessage;
    long long int epoch = 0;
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) pv;
    if (client->run){
        ESP_LOGI(TAG, "El cliente esta corriendo esp_mqtt_task");
    }
    client->run = true;

    client->transport = transport_list_get_transport(client->transport_list, client->config->scheme);

    if (client->transport == NULL) {
        ESP_LOGE(TAG, "There are no transports valid, stop mqtt client, config scheme = %s", client->config->scheme);
        client->run = false;
    }
    if (client->config->port == 0) {
        client->config->port = transport_get_default_port(client->transport);
    }

    client->state = MQTT_STATE_INIT;
    xEventGroupClearBits(client->status_bits, STOPPED_BIT);
    ESP_LOGI(TAG, "Ciclo esp_mqtt_task");
    while (client->run) {
        if (ppposInit() == 0) {
            client->state = MQTT_STATE_INIT;
            client->reconnect_tick = platform_tick_get_ms();
            vTaskDelay(200 / portTICK_RATE_MS);
            continue;
        }
        switch ((int)client->state) {
            case MQTT_STATE_INIT:
                ESP_LOGI(TAG, "MQTT_STATE_INIT");
                if (client->transport == NULL) {
                    ESP_LOGE(TAG, "There are no transport");
                    client->run = false;
                }
                if (transport_connect(client->transport,
                                      client->config->host,
                                      client->config->port,
                                      client->config->network_timeout_ms) < 0) {
                    ESP_LOGE(TAG, "Error transport connect: host %s - port %d - timeout %d", client->config->host,
                                      client->config->port,
                                      client->config->network_timeout_ms);
                    esp_mqtt_abort_connection(client);
                    ppposDisconnect(0,0);
                    break;
                }
                if (esp_mqtt_connect(client, client->config->network_timeout_ms) != ESP_OK) {
                    ESP_LOGI(TAG, "Error MQTT Connected");
                    esp_mqtt_abort_connection(client);
                    ppposDisconnect(0,0);
                    num_intento++;
                    break;
                }
                client->event.event_id = MQTT_EVENT_CONNECTED;
                client->state = MQTT_STATE_CONNECTED;
                esp_mqtt_dispatch_event(client);

                break;
            case MQTT_STATE_CONNECTED:
                if ((msg_id < 0) && (msg_id2 < 0) && mensaje.GPS){
                    vTaskDelay(200 / portTICK_RATE_MS);
                    num_intento++;

                    msgMQTT(&mensaje, pxRxedMessage, num_intento, epoch);
                    msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, mensaje.msg, 0, 0, 0);
                    msg_id2 = esp_mqtt_client_publish(client, MQTT_TOPIC_GPS, mensaje.msgGPS, 0, 0, 0);
                    if (mqtt_process_receive(client) == ESP_FAIL) {
                        esp_mqtt_abort_connection(client);
                        break;
                    }

                    if (platform_tick_get_ms() - client->keepalive_tick > client->connect_info.keepalive * 1000 / 2) {
                        if (esp_mqtt_client_ping(client) == ESP_FAIL) {
                            esp_mqtt_abort_connection(client);
                            break;
                        }
                        client->keepalive_tick = platform_tick_get_ms();
                    }

                    outbox_delete_expired(client->outbox, platform_tick_get_ms(), OUTBOX_EXPIRED_TIMEOUT_MS);
                    outbox_cleanup(client->outbox, OUTBOX_MAX_SIZE);
                } else if ((msg_id2 < 0) && mensaje.GPS){
                    vTaskDelay(200 / portTICK_RATE_MS);
                    num_intento++;

                    msgMQTT(&mensaje, pxRxedMessage, num_intento, epoch);
                    msg_id2 = esp_mqtt_client_publish(client, MQTT_TOPIC_GPS, mensaje.msgGPS, 0, 0, 0);
                    if (mqtt_process_receive(client) == ESP_FAIL) {
                        esp_mqtt_abort_connection(client);
                        break;
                    }

                    if (platform_tick_get_ms() - client->keepalive_tick > client->connect_info.keepalive * 1000 / 2) {
                        if (esp_mqtt_client_ping(client) == ESP_FAIL) {
                            esp_mqtt_abort_connection(client);
                            break;
                        }
                        client->keepalive_tick = platform_tick_get_ms();
                    }

                    outbox_delete_expired(client->outbox, platform_tick_get_ms(), OUTBOX_EXPIRED_TIMEOUT_MS);
                    outbox_cleanup(client->outbox, OUTBOX_MAX_SIZE);

                } else if (msg_id < 0){
                    vTaskDelay(200 / portTICK_RATE_MS);
                    num_intento++;

                    msgMQTT(&mensaje, pxRxedMessage, num_intento, epoch);
                    msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, mensaje.msg, 0, 0, 0);
                    if (mqtt_process_receive(client) == ESP_FAIL) {
                        esp_mqtt_abort_connection(client);
                        break;
                    }

                    if (platform_tick_get_ms() - client->keepalive_tick > client->connect_info.keepalive * 1000 / 2) {
                        if (esp_mqtt_client_ping(client) == ESP_FAIL) {
                            esp_mqtt_abort_connection(client);
                            break;
                        }
                        client->keepalive_tick = platform_tick_get_ms();
                    }

                    outbox_delete_expired(client->outbox, platform_tick_get_ms(), OUTBOX_EXPIRED_TIMEOUT_MS);
                    outbox_cleanup(client->outbox, OUTBOX_MAX_SIZE);

                } else {
                    num_intento = 1;
                    now = 0;
                    struct tm timeinfo = { 0 };
                    int retry = 0;
                    const int retry_count = 10;
                    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
                        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                        time(&now);
                        localtime_r(&now, &timeinfo);
                    }
                    localtime_r(&now, &timeinfo);
                    vTaskDelay(200 / portTICK_RATE_MS);
                    epoch = (long long int)now;

                    if( xQueueReceive( client->queue, &( pxRxedMessage ), ( TickType_t ) 10 ))
                    {
                        msgMQTT(&mensaje, pxRxedMessage, num_intento, epoch);

                        msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, mensaje.msg, 0, 0, 0);
                        if (mensaje.GPS){
                            msg_id2 = esp_mqtt_client_publish(client, MQTT_TOPIC_GPS, mensaje.msgGPS, 0, 0, 0);
                        }
                        if (mqtt_process_receive(client) == ESP_FAIL) {
                            esp_mqtt_abort_connection(client);
                            break;
                        }

                        if (platform_tick_get_ms() - client->keepalive_tick > client->connect_info.keepalive * 1000 / 2) {
                            if (esp_mqtt_client_ping(client) == ESP_FAIL) {
                                esp_mqtt_abort_connection(client);
                                break;
                            }
                            client->keepalive_tick = platform_tick_get_ms();
                        }

                        outbox_delete_expired(client->outbox, platform_tick_get_ms(), OUTBOX_EXPIRED_TIMEOUT_MS);
                        outbox_cleanup(client->outbox, OUTBOX_MAX_SIZE);
                    } else {
                        msg_id = 0;
                        msg_id2 = 0;
                    }
                }

                break;
            case MQTT_STATE_WAIT_TIMEOUT:

                if (!client->config->auto_reconnect) {
                    client->run = false;
                    break;
                }
                if (platform_tick_get_ms() - client->reconnect_tick > client->wait_timeout_ms) {
                    client->state = MQTT_STATE_INIT;
                    client->reconnect_tick = platform_tick_get_ms();
                    ESP_LOGD(TAG, "Reconnecting...");
                }
                vTaskDelay(client->wait_timeout_ms / 2 / portTICK_RATE_MS);
                break;
        }
    }
    transport_close(client->transport);
    xEventGroupSetBits(client->status_bits, STOPPED_BIT);

    vTaskDelete(NULL);
}

esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client)
{
    if (client->state >= MQTT_STATE_INIT) {
        ESP_LOGE(TAG, "Client has started");
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Client has not started %s", getClientState(client->state));
    }
    //initialize_sntp();
    if (xTaskCreate(esp_mqtt_task, "mqtt_task", client->config->task_stack, client, client->config->task_prio, NULL) != pdTRUE) {
        ESP_LOGE(TAG, "Error create mqtt task");
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "Create mqtt task");
    }
    xEventGroupClearBits(client->status_bits, STOPPED_BIT);
    return ESP_OK;
}


esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client)
{
    client->run = false;
    xEventGroupWaitBits(client->status_bits, STOPPED_BIT, false, true, portMAX_DELAY);
    client->state = MQTT_STATE_UNKNOWN;
    return ESP_OK;
}

static esp_err_t esp_mqtt_client_ping(esp_mqtt_client_handle_t client)
{
    client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);

    if (mqtt_write_data(client) != ESP_OK) {
        ESP_LOGE(TAG, "Error sending ping");
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "Sent PING successful");
    return ESP_OK;
}

int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos)
{
    if (client->state != MQTT_STATE_CONNECTED) {
        ESP_LOGE(TAG, "Client has not connected");
        return -1;
    }
    mqtt_enqueue(client); //move pending msg to outbox (if have)
    client->mqtt_state.outbound_message = mqtt_msg_subscribe(&client->mqtt_state.mqtt_connection,
                                          topic, qos,
                                          &client->mqtt_state.pending_msg_id);

    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_count ++;

    if (mqtt_write_data(client) != ESP_OK) {
        ESP_LOGE(TAG, "Error to subscribe topic=%s, qos=%d", topic, qos);
        return -1;
    }

    ESP_LOGD(TAG, "Sent subscribe topic=%s, id: %d, type=%d successful", topic, client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    return client->mqtt_state.pending_msg_id;
}

int esp_mqtt_client_unsubscribe(esp_mqtt_client_handle_t client, const char *topic)
{
    if (client->state != MQTT_STATE_CONNECTED) {
        ESP_LOGE(TAG, "Client has not connected");
        return -1;
    }
    mqtt_enqueue(client);
    client->mqtt_state.outbound_message = mqtt_msg_unsubscribe(&client->mqtt_state.mqtt_connection,
                                          topic,
                                          &client->mqtt_state.pending_msg_id);
    ESP_LOGD(TAG, "unsubscribe, topic\"%s\", id: %d", topic, client->mqtt_state.pending_msg_id);

    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_count ++;

    if (mqtt_write_data(client) != ESP_OK) {
        ESP_LOGE(TAG, "Error to unsubscribe topic=%s", topic);
        return -1;
    }

    ESP_LOGD(TAG, "Sent Unsubscribe topic=%s, id: %d, successful", topic, client->mqtt_state.pending_msg_id);
    return client->mqtt_state.pending_msg_id;
}

int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain)
{
    uint16_t pending_msg_id = 0;
    if (client->state != MQTT_STATE_CONNECTED) {
        ESP_LOGE(TAG, "Client has not connected");
        return -1;
    }
    if (len <= 0) {
        len = strlen(data);
    }
    if (qos > 0) {
        mqtt_enqueue(client);
    }

    client->mqtt_state.outbound_message = mqtt_msg_publish(&client->mqtt_state.mqtt_connection,
                                          topic, data, len,
                                          qos, retain,
                                          &pending_msg_id);
    if (qos > 0) {
        client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
        client->mqtt_state.pending_msg_id = pending_msg_id;
        client->mqtt_state.pending_msg_count ++;
    }

    if (mqtt_write_data(client) != ESP_OK) {
        ESP_LOGE(TAG, "Error to public data to topic=%s, qos=%d", topic, qos);
        return -1;
    }
    return pending_msg_id;
}


