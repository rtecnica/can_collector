# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/viki/.local/share/JetBrains/Toolbox/apps/CLion/ch-0/183.5429.37/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /home/viki/.local/share/JetBrains/Toolbox/apps/CLion/ch-0/183.5429.37/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/viki/CLionProjects/can_collector

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/viki/CLionProjects/can_collector/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/can_collector.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/can_collector.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/can_collector.dir/flags.make

CMakeFiles/can_collector.dir/main/can_collector.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/main/can_collector.c.o: ../main/can_collector.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/can_collector.dir/main/can_collector.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/main/can_collector.c.o   -c /home/viki/CLionProjects/can_collector/main/can_collector.c

CMakeFiles/can_collector.dir/main/can_collector.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/main/can_collector.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/main/can_collector.c > CMakeFiles/can_collector.dir/main/can_collector.c.i

CMakeFiles/can_collector.dir/main/can_collector.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/main/can_collector.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/main/can_collector.c -o CMakeFiles/can_collector.dir/main/can_collector.c.s

CMakeFiles/can_collector.dir/components/can_collector_utils.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/can_collector_utils.c.o: ../components/can_collector_utils.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/can_collector.dir/components/can_collector_utils.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/can_collector_utils.c.o   -c /home/viki/CLionProjects/can_collector/components/can_collector_utils.c

CMakeFiles/can_collector.dir/components/can_collector_utils.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/can_collector_utils.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/can_collector_utils.c > CMakeFiles/can_collector.dir/components/can_collector_utils.c.i

CMakeFiles/can_collector.dir/components/can_collector_utils.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/can_collector_utils.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/can_collector_utils.c -o CMakeFiles/can_collector.dir/components/can_collector_utils.c.s

CMakeFiles/can_collector.dir/components/elm327.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/elm327.c.o: ../components/elm327.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/can_collector.dir/components/elm327.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/elm327.c.o   -c /home/viki/CLionProjects/can_collector/components/elm327.c

CMakeFiles/can_collector.dir/components/elm327.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/elm327.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/elm327.c > CMakeFiles/can_collector.dir/components/elm327.c.i

CMakeFiles/can_collector.dir/components/elm327.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/elm327.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/elm327.c -o CMakeFiles/can_collector.dir/components/elm327.c.s

CMakeFiles/can_collector.dir/components/L80GPS.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/L80GPS.c.o: ../components/L80GPS.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/can_collector.dir/components/L80GPS.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/L80GPS.c.o   -c /home/viki/CLionProjects/can_collector/components/L80GPS.c

CMakeFiles/can_collector.dir/components/L80GPS.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/L80GPS.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/L80GPS.c > CMakeFiles/can_collector.dir/components/L80GPS.c.i

CMakeFiles/can_collector.dir/components/L80GPS.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/L80GPS.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/L80GPS.c -o CMakeFiles/can_collector.dir/components/L80GPS.c.s

CMakeFiles/can_collector.dir/components/libGSM.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/libGSM.c.o: ../components/libGSM.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/can_collector.dir/components/libGSM.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/libGSM.c.o   -c /home/viki/CLionProjects/can_collector/components/libGSM.c

CMakeFiles/can_collector.dir/components/libGSM.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/libGSM.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/libGSM.c > CMakeFiles/can_collector.dir/components/libGSM.c.i

CMakeFiles/can_collector.dir/components/libGSM.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/libGSM.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/libGSM.c -o CMakeFiles/can_collector.dir/components/libGSM.c.s

CMakeFiles/can_collector.dir/components/mqtt_client.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/mqtt_client.c.o: ../components/mqtt_client.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/can_collector.dir/components/mqtt_client.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/mqtt_client.c.o   -c /home/viki/CLionProjects/can_collector/components/mqtt_client.c

CMakeFiles/can_collector.dir/components/mqtt_client.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/mqtt_client.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/mqtt_client.c > CMakeFiles/can_collector.dir/components/mqtt_client.c.i

CMakeFiles/can_collector.dir/components/mqtt_client.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/mqtt_client.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/mqtt_client.c -o CMakeFiles/can_collector.dir/components/mqtt_client.c.s

CMakeFiles/can_collector.dir/components/parse_utils.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/parse_utils.c.o: ../components/parse_utils.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/can_collector.dir/components/parse_utils.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/parse_utils.c.o   -c /home/viki/CLionProjects/can_collector/components/parse_utils.c

CMakeFiles/can_collector.dir/components/parse_utils.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/parse_utils.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/parse_utils.c > CMakeFiles/can_collector.dir/components/parse_utils.c.i

CMakeFiles/can_collector.dir/components/parse_utils.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/parse_utils.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/parse_utils.c -o CMakeFiles/can_collector.dir/components/parse_utils.c.s

CMakeFiles/can_collector.dir/components/SIM_utils.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/SIM_utils.c.o: ../components/SIM_utils.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/can_collector.dir/components/SIM_utils.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/SIM_utils.c.o   -c /home/viki/CLionProjects/can_collector/components/SIM_utils.c

CMakeFiles/can_collector.dir/components/SIM_utils.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/SIM_utils.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/SIM_utils.c > CMakeFiles/can_collector.dir/components/SIM_utils.c.i

CMakeFiles/can_collector.dir/components/SIM_utils.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/SIM_utils.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/SIM_utils.c -o CMakeFiles/can_collector.dir/components/SIM_utils.c.s

CMakeFiles/can_collector.dir/components/stack_utils.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/stack_utils.c.o: ../components/stack_utils.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object CMakeFiles/can_collector.dir/components/stack_utils.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/stack_utils.c.o   -c /home/viki/CLionProjects/can_collector/components/stack_utils.c

CMakeFiles/can_collector.dir/components/stack_utils.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/stack_utils.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/stack_utils.c > CMakeFiles/can_collector.dir/components/stack_utils.c.i

CMakeFiles/can_collector.dir/components/stack_utils.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/stack_utils.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/stack_utils.c -o CMakeFiles/can_collector.dir/components/stack_utils.c.s

CMakeFiles/can_collector.dir/components/lib/transport_ws.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/lib/transport_ws.c.o: ../components/lib/transport_ws.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object CMakeFiles/can_collector.dir/components/lib/transport_ws.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/lib/transport_ws.c.o   -c /home/viki/CLionProjects/can_collector/components/lib/transport_ws.c

CMakeFiles/can_collector.dir/components/lib/transport_ws.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/lib/transport_ws.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/lib/transport_ws.c > CMakeFiles/can_collector.dir/components/lib/transport_ws.c.i

CMakeFiles/can_collector.dir/components/lib/transport_ws.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/lib/transport_ws.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/lib/transport_ws.c -o CMakeFiles/can_collector.dir/components/lib/transport_ws.c.s

CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.o: ../components/lib/transport_tcp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.o   -c /home/viki/CLionProjects/can_collector/components/lib/transport_tcp.c

CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/lib/transport_tcp.c > CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.i

CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/lib/transport_tcp.c -o CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.s

CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.o: ../components/lib/transport_ssl.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.o   -c /home/viki/CLionProjects/can_collector/components/lib/transport_ssl.c

CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/lib/transport_ssl.c > CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.i

CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/lib/transport_ssl.c -o CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.s

CMakeFiles/can_collector.dir/components/lib/transport.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/lib/transport.c.o: ../components/lib/transport.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object CMakeFiles/can_collector.dir/components/lib/transport.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/lib/transport.c.o   -c /home/viki/CLionProjects/can_collector/components/lib/transport.c

CMakeFiles/can_collector.dir/components/lib/transport.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/lib/transport.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/lib/transport.c > CMakeFiles/can_collector.dir/components/lib/transport.c.i

CMakeFiles/can_collector.dir/components/lib/transport.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/lib/transport.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/lib/transport.c -o CMakeFiles/can_collector.dir/components/lib/transport.c.s

CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.o: ../components/lib/platform_esp32_idf.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building C object CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.o   -c /home/viki/CLionProjects/can_collector/components/lib/platform_esp32_idf.c

CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/lib/platform_esp32_idf.c > CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.i

CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/lib/platform_esp32_idf.c -o CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.s

CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.o: ../components/lib/mqtt_outbox.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Building C object CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.o   -c /home/viki/CLionProjects/can_collector/components/lib/mqtt_outbox.c

CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/lib/mqtt_outbox.c > CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.i

CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/lib/mqtt_outbox.c -o CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.s

CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.o: ../components/lib/mqtt_msg.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_16) "Building C object CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.o   -c /home/viki/CLionProjects/can_collector/components/lib/mqtt_msg.c

CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/lib/mqtt_msg.c > CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.i

CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/lib/mqtt_msg.c -o CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.s

CMakeFiles/can_collector.dir/components/OTA_utils.c.o: CMakeFiles/can_collector.dir/flags.make
CMakeFiles/can_collector.dir/components/OTA_utils.c.o: ../components/OTA_utils.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_17) "Building C object CMakeFiles/can_collector.dir/components/OTA_utils.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/can_collector.dir/components/OTA_utils.c.o   -c /home/viki/CLionProjects/can_collector/components/OTA_utils.c

CMakeFiles/can_collector.dir/components/OTA_utils.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/can_collector.dir/components/OTA_utils.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/viki/CLionProjects/can_collector/components/OTA_utils.c > CMakeFiles/can_collector.dir/components/OTA_utils.c.i

CMakeFiles/can_collector.dir/components/OTA_utils.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/can_collector.dir/components/OTA_utils.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/viki/CLionProjects/can_collector/components/OTA_utils.c -o CMakeFiles/can_collector.dir/components/OTA_utils.c.s

# Object files for target can_collector
can_collector_OBJECTS = \
"CMakeFiles/can_collector.dir/main/can_collector.c.o" \
"CMakeFiles/can_collector.dir/components/can_collector_utils.c.o" \
"CMakeFiles/can_collector.dir/components/elm327.c.o" \
"CMakeFiles/can_collector.dir/components/L80GPS.c.o" \
"CMakeFiles/can_collector.dir/components/libGSM.c.o" \
"CMakeFiles/can_collector.dir/components/mqtt_client.c.o" \
"CMakeFiles/can_collector.dir/components/parse_utils.c.o" \
"CMakeFiles/can_collector.dir/components/SIM_utils.c.o" \
"CMakeFiles/can_collector.dir/components/stack_utils.c.o" \
"CMakeFiles/can_collector.dir/components/lib/transport_ws.c.o" \
"CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.o" \
"CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.o" \
"CMakeFiles/can_collector.dir/components/lib/transport.c.o" \
"CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.o" \
"CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.o" \
"CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.o" \
"CMakeFiles/can_collector.dir/components/OTA_utils.c.o"

# External object files for target can_collector
can_collector_EXTERNAL_OBJECTS =

can_collector: CMakeFiles/can_collector.dir/main/can_collector.c.o
can_collector: CMakeFiles/can_collector.dir/components/can_collector_utils.c.o
can_collector: CMakeFiles/can_collector.dir/components/elm327.c.o
can_collector: CMakeFiles/can_collector.dir/components/L80GPS.c.o
can_collector: CMakeFiles/can_collector.dir/components/libGSM.c.o
can_collector: CMakeFiles/can_collector.dir/components/mqtt_client.c.o
can_collector: CMakeFiles/can_collector.dir/components/parse_utils.c.o
can_collector: CMakeFiles/can_collector.dir/components/SIM_utils.c.o
can_collector: CMakeFiles/can_collector.dir/components/stack_utils.c.o
can_collector: CMakeFiles/can_collector.dir/components/lib/transport_ws.c.o
can_collector: CMakeFiles/can_collector.dir/components/lib/transport_tcp.c.o
can_collector: CMakeFiles/can_collector.dir/components/lib/transport_ssl.c.o
can_collector: CMakeFiles/can_collector.dir/components/lib/transport.c.o
can_collector: CMakeFiles/can_collector.dir/components/lib/platform_esp32_idf.c.o
can_collector: CMakeFiles/can_collector.dir/components/lib/mqtt_outbox.c.o
can_collector: CMakeFiles/can_collector.dir/components/lib/mqtt_msg.c.o
can_collector: CMakeFiles/can_collector.dir/components/OTA_utils.c.o
can_collector: CMakeFiles/can_collector.dir/build.make
can_collector: CMakeFiles/can_collector.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_18) "Linking C executable can_collector"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/can_collector.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/can_collector.dir/build: can_collector

.PHONY : CMakeFiles/can_collector.dir/build

CMakeFiles/can_collector.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/can_collector.dir/cmake_clean.cmake
.PHONY : CMakeFiles/can_collector.dir/clean

CMakeFiles/can_collector.dir/depend:
	cd /home/viki/CLionProjects/can_collector/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/viki/CLionProjects/can_collector /home/viki/CLionProjects/can_collector /home/viki/CLionProjects/can_collector/cmake-build-debug /home/viki/CLionProjects/can_collector/cmake-build-debug /home/viki/CLionProjects/can_collector/cmake-build-debug/CMakeFiles/can_collector.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/can_collector.dir/depend

