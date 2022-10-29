cmake_minimum_required(VERSION 3.10)

project(athrill2)

set(CORE_DIR "athrill/src")
set(APLDIR "athrill/apl")
set(TARGET_DIR "src")

include_directories("${CORE_DIR}/inc" "${CORE_DIR}/lib" )

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DOS_LINUX -DSUPRESS_DETECT_WARNING_MESSAGE)
    message(STATUS "CMAKE_HOST_SYSTEM_NAME is Linux")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(-DOS_LINUX -DOS_MAC -DSUPRESS_DETECT_WARNING_MESSAGE)
    message(STATUS "CMAKE_HOST_SYSTEM_NAME is Darwin")
else()
    add_definitions()
    message(STATUS "CMAKE_HOST_SYSTEM_NAME is others")
endif()

add_definitions(-D DISABLE_DEVICE_VDEV -D CPUEMU_CLOCK_BUG_FIX -DSEARCH_REGION_OPTIMIZE -DCPU_PERMISSION_OPTIMIZE -DDISABLE_SERIAL_CLOCK -DDISABLE_BUS_ACCESS_LOG -DOPTIMIZE_USE_ONLY_1CPU -DMINIMUM_DEVICE_CONFIG -DENABLE_REUSE_UDP_SOCKET -DEXDEV_ENABLE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -g -Wall -Wunknown-pragmas -Wimplicit-int -Wtrigraphs -std=gnu99 -O3")


add_library(main)
target_include_directories(main PUBLIC  
    "${CORE_DIR}/lib/dwarf" 
    "${CORE_DIR}/bus" 
    "${CORE_DIR}/device/mpu" 
    "${CORE_DIR}/debugger/interaction" 
    "${CORE_DIR}/debugger/interaction/inc" 
    "${CORE_DIR}/debugger/executor"
    "${CORE_DIR}/cpu"
    "${CORE_DIR}/main"
    "${TARGET_DIR}/cpu" 
    "${TARGET_DIR}/cpu/config" 
    "${TARGET_DIR}/debugger/interaction/inc" 
)
target_sources(main PRIVATE
    "${CORE_DIR}/main/main.c"
    "${CORE_DIR}/main/cpuemu.c"
    "${CORE_DIR}/main/option/option.c"
    "${CORE_DIR}/debugger/executor/cpu_control/dbg_cpu_callback.c"
    "${CORE_DIR}/debugger/executor/cpu_control/dbg_cpu_control.c"
    "${CORE_DIR}/debugger/executor/cpu_control/dbg_cpu_thread_control.c"
)


add_library(cui)
target_include_directories(cui PRIVATE  
    "${CORE_DIR}/lib/dwarf" 
    "${CORE_DIR}/debugger/interaction" 
    "${CORE_DIR}/debugger/interaction/inc" 
    "${CORE_DIR}/debugger/executor" 
    "${CORE_DIR}/debugger/executor/concrete_executor/target"
    "${CORE_DIR}/cpu"
    "${TARGET_DIR}/cpu" 
    "${TARGET_DIR}/cpu/config" 
    "${TARGET_DIR}/debugger/interaction/inc" 
)
target_sources(cui PRIVATE
    "${CORE_DIR}/lib/cui/cui_ops.c" 
    "${CORE_DIR}/lib/cui/stdio/cui_ops_stdio.c" 
    "${CORE_DIR}/lib/cui/udp/cui_ops_udp.c"
    "${CORE_DIR}/debugger/interaction/front/parser/dbg_parser_config.c" 
    "${CORE_DIR}/debugger/interaction/front/parser/dbg_parser.c" 
    "${CORE_DIR}/debugger/interaction/front/parser/concrete_parser/dbg_std_parser.c" 
    "${CORE_DIR}/debugger/executor/concrete_executor/dbg_std_executor.c" 
    "${CORE_DIR}/debugger/executor/concrete_executor/util/dbg_print_data_type.c"
)


add_library(cpu)
target_include_directories(cpu PRIVATE  
    "${CORE_DIR}/cpu" 
    "${CORE_DIR}/bus" 
    "${CORE_DIR}/device/mpu" 
    "${TARGET_DIR}/cpu" 
    "${TARGET_DIR}/cpu/config" 
    "${TARGET_DIR}/cpu/cpu_exec" 
    "${TARGET_DIR}/cpu/cpu_dec"
    "${TARGET_DIR}/cpu/cpu_common" 
    "${TARGET_DIR}/device"
)
target_sources(cpu PRIVATE
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_bit.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_branch.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_dbg.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_div.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_logic.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_arithm.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_store.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_load.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_spec.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_fpu.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec.c" 
    "${TARGET_DIR}/cpu/cpu_exec/op_exec_sat.c" 
    "${TARGET_DIR}/cpu/cpu_dec/op_dec.c" 
    "${TARGET_DIR}/cpu/cpu_dec/op_parse.c" 
    "${TARGET_DIR}/cpu/cpu_dec/op_parse_private.c" 
    "${TARGET_DIR}/cpu/config/cpu_config.c"
    "${CORE_DIR}/lib/dbg_log.c"
)


add_library(device)
target_include_directories(device PRIVATE  
    "${CORE_DIR}/cpu" 
    "${CORE_DIR}/device/mpu" 
    "${CORE_DIR}/debugger/executor" 
    "${CORE_DIR}/device/peripheral/target" 
    "${CORE_DIR}/device/peripheral/mros-dev/mros-athrill/api" 
    "${CORE_DIR}/device/peripheral/mros-dev/mros-athrill/device" 
    "${CORE_DIR}/device/peripheral/mros-dev/mros-athrill/target/os" 
    "${CORE_DIR}/device/peripheral/mros-dev/mros-src/os/target/os_asp" 
    "${CORE_DIR}/device/peripheral/serial/fifo" 
    "${APLDIR}/include"
    "${TARGET_DIR}/cpu" 
    "${TARGET_DIR}/cpu/config" 
    "${TARGET_DIR}/device"
    "${TARGET_DIR}/device/intc" 
    "${TARGET_DIR}/device/intc/gic" 
    "${TARGET_DIR}/device/intc/register"
    "${TARGET_DIR}/device/peripheral" 
    "${TARGET_DIR}/device/peripheral/inc"
)
target_sources(device PRIVATE
    "${CORE_DIR}/device/peripheral/serial/fifo/serial_fifo.c"
    "${TARGET_DIR}/device/device.c"
    "${TARGET_DIR}/device/intc/intc.c"
    "${TARGET_DIR}/device/peripheral/timer/timer32.c"
    "${TARGET_DIR}/device/peripheral/serial/serial.c"
    "${TARGET_DIR}/device/peripheral/can/can.c"
    "${TARGET_DIR}/debugger/executor/concrete_executor/dbg_target_serial.c"
    "${TARGET_DIR}/debugger/executor/concrete_executor/dbg_target_cpu.c"
)
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
else()
target_sources(device PRIVATE
    "${CORE_DIR}/device/peripheral/athrill_device.c" 
    "${CORE_DIR}/device/peripheral/athrill_syscall_device.c" 
    "${CORE_DIR}/device/peripheral/athrill_mpthread.c" 
)
endif()

add_library(bus)
target_include_directories(bus PRIVATE  
    "${CORE_DIR}/device/mpu" 
)
target_sources(bus PRIVATE
    "${CORE_DIR}/bus/bus.c"
)
# add_subdirectory(${CORE_DIR}/bus)

add_library(loader)
target_include_directories(loader PRIVATE  
    "${CORE_DIR}/lib/dwarf" 
    "${CORE_DIR}/cpu" 
    "${TARGET_DIR}/cpu" 
    "${TARGET_DIR}/cpu/config"
)
target_sources(loader PRIVATE
    "${CORE_DIR}/lib/symbol_ops.c"
    "${CORE_DIR}/lib/dwarf/elf_section.c" 
    "${CORE_DIR}/lib/dwarf/elf_dwarf_line.c" 
    "${CORE_DIR}/lib/dwarf/elf_dwarf_loc.c" 
    "${CORE_DIR}/lib/dwarf/elf_dwarf_info.c" 
    "${CORE_DIR}/lib/dwarf/elf_dwarf_abbrev.c" 
    "${CORE_DIR}/lib/dwarf/elf_dwarf_util.c" 
    "${CORE_DIR}/lib/dwarf/elf_dwarf_info_ops.c" 
    "${CORE_DIR}/lib/dwarf/file_address_mapping.c" 
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_data_type.c"
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_base_type.c"
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_typedef_type.c"
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_pointer_type.c"
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_struct_type.c"
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_enum_type.c"
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_array_type.c"
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_variable_type.c"
    "${CORE_DIR}/lib/dwarf/data_type/elf_dwarf_subprogram_type.c"
)


add_library(mpu ${MPU-SRC})
target_include_directories(mpu PRIVATE  
    "${CORE_DIR}/device/mpu" 
    "${CORE_DIR}/device/peripheral/serial/fifo" 
    "${CORE_DIR}/cpu" 
    "${CORE_DIR}/lib/dwarf" 
    "${CORE_DIR}/lib/dwarf/data_type" 
    "${TARGET_DIR}/cpu" 
    "${TARGET_DIR}/cpu/config" 
)
target_sources(mpu PRIVATE
    "${CORE_DIR}/device/mpu/mpu.c" 
    "${CORE_DIR}/device/mpu/mpu_malloc.c" 
    "${CORE_DIR}/device/mpu/loader/loader.c"
    "${TARGET_DIR}/cpu/config/mpu_config.c"
)


add_library(std)
target_sources(std PRIVATE
    "${CORE_DIR}/lib/hash.c" 
    "${CORE_DIR}/lib/token.c" 
    "${CORE_DIR}/lib/file.c" 
    "${CORE_DIR}/lib/winsock_wrapper/winsock_wrapper.c"
    "${CORE_DIR}/lib/udp/udp_comm.c" 
)
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
else()
target_sources(std PRIVATE
    "${CORE_DIR}/lib/comm_buffer.c" 
    "${CORE_DIR}/lib/tcp/tcp_socket.c"
    "${CORE_DIR}/lib/tcp/tcp_client.c"
    "${CORE_DIR}/lib/tcp/tcp_connection.c"
    "${CORE_DIR}/lib/tcp/tcp_server.c"
)
endif()

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
else()
set(MROS_SRC "${CORE_DIR}/device/peripheral/mros-dev/mros-src")
set(MROS_DEV "${CORE_DIR}/device/peripheral/mros-dev/mros-athrill")
set(ROS_VERSION	"kinetic")

add_library(mros)
target_include_directories(mros PRIVATE  
    "${TARGET_DIR}/cpu" 
    "${TARGET_DIR}/cpu/config"
    "${MROS_SRC}/api" 
    "${MROS_SRC}/inc" 
    "${MROS_SRC}/os/target/os_asp" 
    "${MROS_SRC}/protocol/cimpl" 
    "${MROS_SRC}/node/cimpl"
    "${MROS_SRC}/topic/cimpl" 
    "${MROS_SRC}/comm/cimpl/target/lwip" 
    "${MROS_SRC}/comm/cimpl" 
    "${MROS_SRC}/packet/cimpl"
    "${MROS_SRC}/packet/template/version/${ROS_VERSION}" 
    "${MROS_SRC}/packet/cimpl/version/${ROS_VERSION}" 
    "${MROS_SRC}/transfer/cimpl"
    "${MROS_DEV}/api" 
    "${MROS_DEV}/config" 
    "${MROS_DEV}/config/os/target/os_asp" 
    "${MROS_DEV}/target" 
    "${MROS_DEV}/target/os"
    "${MROS_DEV}/device"
)
target_compile_options(mros PRIVATE
    -DTARGET_ATHRILL
    -DLWIP_TRANSPORT_ETHERNET
)
target_sources(mros PRIVATE
    "${MROS_SRC}/comm/cimpl/mros_comm_socket_cimpl.c"
    "${MROS_SRC}/comm/cimpl/mros_comm_tcp_client_cimpl.c"
    "${MROS_SRC}/comm/cimpl/mros_comm_tcp_client_factory_cimpl.c"
    "${MROS_SRC}/comm/cimpl/mros_comm_tcp_server_cimpl.c"
    "${MROS_SRC}/comm/cimpl/target/lwip/mros_comm_cimpl.c"
    "${MROS_SRC}/lib/mros_memory.c"
    "${MROS_SRC}/lib/mros_wait_queue.c"
    "${MROS_SRC}/lib/mros_memory.c"
    "${MROS_SRC}/lib/mros_name.c"
    "${MROS_SRC}/node/cimpl/mros_node_cimpl.c"
    "${MROS_SRC}/os/mros_exclusive_area.c"
    "${MROS_SRC}/os/target/os_asp/mros_os.c"
    "${MROS_SRC}/packet/cimpl/version/${ROS_VERSION}/mros_packet_decoder_cimpl.c"
    "${MROS_SRC}/packet/cimpl/version/${ROS_VERSION}/mros_packet_encoder_cimpl.c"
    "${MROS_SRC}/protocol/cimpl/mros_protocol_client_rpc_cimpl.c"
    "${MROS_SRC}/protocol/cimpl/mros_protocol_operation_cimpl.c"
    "${MROS_SRC}/protocol/cimpl/mros_protocol_publish_cimpl.c"
    "${MROS_SRC}/protocol/cimpl/mros_protocol_server_proc_cimpl.c"
    "${MROS_SRC}/protocol/cimpl/mros_protocol_slave_cimpl.c"
    "${MROS_SRC}/protocol/cimpl/mros_protocol_subscribe_cimpl.c"
    "${MROS_SRC}/protocol/cimpl/mros_protocol_master_cimpl.c"
    "${MROS_SRC}/topic/cimpl/mros_topic_cimpl.c"
    "${MROS_SRC}/topic/cimpl/mros_topic_connector_cimpl.c"
    "${MROS_SRC}/topic/cimpl/mros_topic_connector_factory_cimpl.c"
    "${MROS_SRC}/transfer/cimpl/mros_topic_data_publisher_cimpl.c"
    "${MROS_SRC}/transfer/cimpl/mros_topic_data_subscriber_cimpl.c"
    "${MROS_SRC}/transfer/cimpl/mros_topic_data_publisher_cimpl.c"
    "${MROS_DEV}/api/ros_cimpl.c"
    "${MROS_DEV}/device/athrill_mros_device.c"
    "${MROS_DEV}/config/mros_sys_config.c"
    "${MROS_DEV}/config/os/target/os_asp/mros_os_config.c"
    "${MROS_DEV}/target/os/os_asp.c"
    "${MROS_DEV}/target/os/mros_exclusive_ops_linux.c"
    "${MROS_DEV}/target/lwip/lwip_linux.c"
    )
endif()

# add_subdirectory(${CORE_DIR}/device/peripheral/mros-dev)

add_executable(athrill2 "${CORE_DIR}/main/main.c")
target_include_directories(athrill2 INTERFACE main)
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
target_link_libraries(athrill2 main cui cpu device bus loader mpu std pthread wsock32)
else()
target_link_libraries(athrill2 main cui cpu device bus loader mpu std mros pthread m dl)
endif()

# install(TARGETS main)

# enable_testing()
# add_test(NAME main COMMAND main)