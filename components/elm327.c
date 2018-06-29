//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//
/**
 * @file
 *
 */
#include "include/elm327.h"
#include "include/parse_utils.h"
#include "include/card_utils.h"

#define MESSAGE_QUEUE_LENGTH 5
static const int RX_BUF_SIZE = 128;

char VIN[17];
static const char *TAG = "CAN_COLLECTOR";

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            //msg_id = esp_mqtt_client_subscribe(client, "esp32", 0);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            msg_id = esp_mqtt_client_publish(client, "esp32", "data", 0, 0, 0);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

static esp_mqtt_client_handle_t mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://172.16.127.182:1883",
        .event_handle = mqtt_event_handler,
        // .user_context = (void *)your_context
    };
    ESP_LOGI(TAG, "Iniciando el cliente en mqtt_app_start");
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_LOGI(TAG, "Creado el cliente en mqtt_app_start");
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "Iniciado el cliente en mqtt_app_start");
    return client;
}

static void mqtt_app_stop(esp_mqtt_client_handle_t cliente)
{
    esp_mqtt_client_stop(cliente);
}

//Proceso de consumo de mensajes en queue
void mqtt_msg_task(void *pvParameters) {
    char msg[20];
    char msgError[20];
    char strIntento[10];
    int num_intento = 0;
    int msg_id;
    esp_mqtt_client_handle_t client;
    for(;;) {
        vTaskDelay(200 / portTICK_RATE_MS);
        ESP_LOGI(TAG, "Iniciando el cliente en mqtt_msg_task");
        client = mqtt_app_start();
        ESP_LOGI(TAG, "Iniciado el cliente en mqtt_msg_task");
        while (1){
            vTaskDelay(2000 / portTICK_RATE_MS);
            if (num_intento == 0){ //Aqui se hace la consulta al lector del can bus, se debe consumir la cola
                strcpy(msg, "intento=");
                itoa(num_intento, strIntento, 10);
                strcat(msg, strIntento);
                strcat(msg, ",client=prueba1,gps(lat=20.3,lon=21.3),vel=23.4,comb=50.5");
                ESP_LOGI(TAG, "Publicacion MQTT %s mqtt_msg_task", msg);
                strcpy(msg, "Hola");
                msg_id = esp_mqtt_client_publish(client, "esp32", msg, 0, 0, 0);
                ESP_LOGI(TAG, "Mensaje publicado. El id del mensaje es:[%d] mqtt_msg_task", msg_id);
            }
            vTaskDelay(2000 / portTICK_RATE_MS);
            if (msg_id < 0){
                num_intento++;
                strcpy(msg, "intento=");
                itoa(num_intento, strIntento, 10);
                strcat(msg, strIntento);
                strcat(msg, ",client=prueba1,gps(lat=20.3,lon=21.3),vel=23.4,comb=50.5");
                msg_id = esp_mqtt_client_publish(client, "esp32", msg, 0, 0, 0);
                ESP_LOGI(TAG, "Entro al error: intento %d", num_intento);
                break;
            } else {
                ESP_LOGI(TAG, "Volvio del error");
                num_intento = 0;
            }
            if (num_intento > 0 ){
                strcpy(msgError, "Error=1,intentos=");
                itoa(num_intento, strIntento, 10);
                strcat(msgError, strIntento);
                esp_mqtt_client_publish(client, "esp32_error", msgError, 0, 0, 0);
                continue;
            }
        }
        mqtt_app_stop(client);
    }
    //vPortFree(data);
    vTaskDelete(NULL);
}

//Proceso de monitoreo de interfase UART
void elm327_rx_task(void *pvParameters) {
    for(;;) {
        uint8_t* data = (uint8_t*) pvPortMalloc(RX_BUF_SIZE+1);
        while(data == NULL){
            ESP_LOGI("RX_TASK","Waiting for available heap space...");
            vTaskDelay(100/portTICK_PERIOD_MS);
            data = (uint8_t*) pvPortMalloc(RX_BUF_SIZE+1);
        }
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 500 / portTICK_RATE_MS);
        if (rxBytes > 0) {

            data[rxBytes] = 0;
            ESP_LOGI("RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP("RX_TASK_HEXDUMP", data, rxBytes, ESP_LOG_INFO);

            //Send through queue to data processing Task
            esp_spp_write(*((( struct param *)pvParameters)->out_bt_handle),rxBytes,data);

            xQueueSend((( struct param *)pvParameters)->rxQueue,(void *)(&data),0);
            //vPortFree(data); // data will be vPortFreed by recieving function
        }
        else{
            vPortFree(data);
        }
    }
    //vPortFree(data);
    vTaskDelete(NULL);
}

void elm327_parse_task(void *pvParameters){

    void **buff = pvPortMalloc(sizeof(void *));
    BaseType_t xStatus;
    can_msg_t msg_type;

    elm327_data_t packet;

    for(;;){
        xStatus = xQueueReceive(((struct param *)pvParameters)->rxQueue, buff, 100/portTICK_PERIOD_MS);
        if(xStatus == pdPASS) {
            msg_type = parse_check_msg_type((uint8_t *)*buff,6);
            ESP_LOGI("PARSE_TASK", "Message Type Recieved: %04x", msg_type);
            if(parse_is_data((uint8_t *)(*buff))){
                uint8_t var = (((uint8_t)parse_char_to_hex(((uint8_t *)(*buff))[11]))<<4) + ((uint8_t)parse_char_to_hex(((uint8_t *)(*buff))[12]));
                switch(msg_type){
                    case FUELTANK_MSG:
                        packet.fuel = var;
                        packet.fields = packet.fields | FUEL_FIELD;
                        ESP_LOGI("FUELTANK_MSG","Fuel Level = %02x",var);
                        break;
                    case OILTEMP_MSG:
                        packet.temp = var;
                        packet.fields = packet.fields | TEMP_FIELD;
                        ESP_LOGI("OILTEMP_MSG","Oil Temp = %02x",var);
                        break;
                    case SPEED_MSG:
                        packet.speed = var;
                        packet.fields = packet.fields | SPEED_FIELD;
                        ESP_LOGI("SPEED_MSG","Speed = %02x",var);
                        break;
                    case VIN_MSG:
                        vin_parse(VIN,*buff);
                        packet.fields = packet.fields | VIN_FIELD;
                        ESP_LOGI("VIN_MSG","VIN = %s",VIN);
                        break;
                    case UNKNOWN_MSG:
                        break;
                }
            }
            else{
                ESP_LOGI("PARSE_TASK", "No Data!");
            }

            vPortFree(*buff);

            if((packet.fields & (VIN_FIELD | SPEED_FIELD | FUEL_FIELD | TEMP_FIELD | MISC_FIELD)) == (VIN_FIELD | SPEED_FIELD | FUEL_FIELD | TEMP_FIELD | MISC_FIELD) ){
                ESP_LOGI("PARSE_TASK","Packet Ready for Sending");

                if(xQueueSend(((struct param *)pvParameters)->OutQueue,&packet,0) == pdPASS){
                    ESP_LOGI("PARSE_TASK","Packet sent to Outgoing Queue");
                }
                else{
                    if(xQueueSend(((struct param *)pvParameters)->storeQueue,&packet,0) == pdPASS){
                        ESP_LOGI("PARSE_TASK","Packet sent to Storage Queue");
                    }
                    else{
                        ESP_LOGI("PARSE_TASK","Storage Queue Full!");
                    }
                }
                packet.fields = VIN_FIELD | MISC_FIELD;
            }
        }
    }
    vTaskDelete(NULL);
}

// Inicializa el módulo UART #0 que está conectalo a la interfase USB-UART
void elm327_init(uint32_t *bt_handle) {
    const uart_config_t uart_config = {
            .baud_rate = 38400,//115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);

    vParams.out_bt_handle = bt_handle;

    vParams.rxQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(void *));
    if(vParams.rxQueue != (NULL)){
        ESP_LOGI("RX_QUEUE", "rxQueue creation successful");
    }

    vParams.OutQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(elm327_data_t));
    if(vParams.OutQueue != (NULL)){
        ESP_LOGI("OUT_QUEUE", "OutQueue creation successful");
    }

    vParams.storeQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(elm327_data_t));
    if(vParams.storeQueue != (NULL)){
        ESP_LOGI("STORE_QUEUE", "storeQueue creation successful");
    }

    xTaskCreate(elm327_rx_task, "elm327_rx_task", 1024 * 2, (void *)&vParams, configMAX_PRIORITIES, NULL);
    xTaskCreate(elm327_parse_task, "elm327_parse_task", 1024 * 2, (void *)&vParams, configMAX_PRIORITIES - 1, NULL);
 
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    nvs_flash_init();
    wifi_init();

    xTaskCreate(mqtt_msg_task, "mqtt_msg_task", 4096, (void *)&vParams, configMAX_PRIORITIES - 1, NULL);
}

//Función de utilidad para enviar bytestream a través de UART al ELM327
bool elm327_sendData(const char* logName, unsigned char* data, const int len) {
    union Data{
        const char *string;
        unsigned char *raw;
    }uni;

    uni.raw = data;
    
    const int txBytes = uart_write_bytes(UART_NUM_1, uni.string, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    ESP_LOG_BUFFER_HEXDUMP(logName, data, txBytes, ESP_LOG_INFO);

    return txBytes == len;
}

bool elm327_reset(void){
    unsigned char msg[5] = {0x41, 0x54, 0x20, 0x5a, 0x0d};
    return elm327_sendData("Reset", msg, 5);
}

bool elm327_setCAN(void){
    unsigned char msg2[9] = {0x41, 0x54, 0x20, 0x54, 0x50, 0x20, 0x36, 0x0d};
    return elm327_sendData("Set Protocol", msg2, 8);
};

bool elm327_query_oiltemp(void){

    unsigned char msg[5] = {0x30, 0x31, 0x35, 0x43, 0x0d};

    return elm327_sendData("OilTemp Query", msg, 5);
}

bool elm327_query_fueltank(void){

    unsigned char msg[5] = {0x30, 0x31, 0x32, 0x46, 0x0d};

    return elm327_sendData("FuelTank Query", msg, 5);
}

bool elm327_query_speed(void){

    unsigned char msg[5] = {0x30, 0x31, 0x30, 0x44, 0x0d};

    return elm327_sendData("Speed Query", msg, 5);
}

bool elm327_query_VIN(void){
    unsigned char msg[5] = {0x30, 0x39, 0x30, 0x32, 0x0d};

    return elm327_sendData("VIN Query", msg, 5);
}

bool elm327_query_GPS(void){
    //TODO Write this function
    return 1;
}

void elm327_new_data(elm327_data_t *data){

    strcpy(data->VIN, "AAAAAAAAAAAAAAAAA");
    strcpy(data->LAT, "00000000");
    strcpy(data->LONG, "00000000");
    strcpy(data->TIME, "0000");
    data->temp = 0x46;
    data->fuel = 0x46;
    data->speed = 0x46;
    data->fields = ALL_FIELDS;
}