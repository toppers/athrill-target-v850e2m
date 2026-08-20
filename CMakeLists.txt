cmake_minimum_required(VERSION 3.16)

project(athrill2 VERSION 1.0.2 LANGUAGES C)

include(GNUInstallDirs)
include(CTest)

if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR
            "The supported native Windows toolchain is MSVC. "
            "Configure from PowerShell with the vcpkg toolchain.")
    endif()
    find_package(PThreads4W CONFIG REQUIRED)
else()
    find_package(Threads REQUIRED)
endif()

set(ATHRILL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/athrill"
    CACHE PATH "Path to the Athrill common source repository")
get_filename_component(ATHRILL_SOURCE_DIR "${ATHRILL_SOURCE_DIR}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(_athrill_cmake_module "${ATHRILL_SOURCE_DIR}/cmake/AthrillCore.cmake")
if(NOT EXISTS "${_athrill_cmake_module}")
    message(FATAL_ERROR
        "Athrill common CMake module was not found: ${_athrill_cmake_module}\n"
        "Set ATHRILL_SOURCE_DIR to a compatible Athrill checkout.")
endif()
include("${_athrill_cmake_module}")

set(ATHRILL_TARGET_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

option(ATHRILL_ENABLE_EXDEV "Enable external device loading" ON)
option(ATHRILL_ENABLE_MROS "Build the legacy mROS integration" OFF)
option(ATHRILL_ENABLE_VDEV "Enable the legacy VDEV implementation" OFF)

if(ATHRILL_ENABLE_VDEV)
    message(FATAL_ERROR "ATHRILL_ENABLE_VDEV is not yet supported by the CMake build")
endif()

add_library(athrill_v850_build_config INTERFACE)
target_compile_definitions(athrill_v850_build_config INTERFACE
    CPUEMU_CLOCK_BUG_FIX
    SEARCH_REGION_OPTIMIZE
    CPU_PERMISSION_OPTIMIZE
    DISABLE_SERIAL_CLOCK
    DISABLE_BUS_ACCESS_LOG
    OPTIMIZE_USE_ONLY_1CPU
    MINIMUM_DEVICE_CONFIG
    ENABLE_REUSE_UDP_SOCKET
)
if(ATHRILL_ENABLE_EXDEV)
    target_compile_definitions(athrill_v850_build_config INTERFACE EXDEV_ENABLE)
endif()
if(NOT ATHRILL_ENABLE_MROS)
    target_compile_definitions(athrill_v850_build_config INTERFACE DISABLE_CAN)
endif()
if(NOT ATHRILL_ENABLE_VDEV)
    target_compile_definitions(athrill_v850_build_config INTERFACE DISABLE_DEVICE_VDEV)
endif()

add_library(athrill_host_platform INTERFACE)
if(WIN32)
    target_compile_definitions(athrill_host_platform INTERFACE PTW32_STATIC_LIB)
    target_link_libraries(athrill_host_platform INTERFACE
        PThreads4W::PThreads4W
        ws2_32
    )
else()
    target_link_libraries(athrill_host_platform INTERFACE
        Threads::Threads
        m
        ${CMAKE_DL_LIBS}
    )
endif()

function(athrill_v850_configure_target target_name)
    athrill_apply_common_settings("${target_name}")
    target_link_libraries("${target_name}" PRIVATE
        athrill_v850_build_config
        athrill_host_platform
    )
endfunction()

add_library(athrill_main STATIC ${ATHRILL_MAIN_SOURCES})
target_include_directories(athrill_main PRIVATE
    "${ATHRILL_SRC_DIR}/lib/dwarf"
    "${ATHRILL_SRC_DIR}/bus"
    "${ATHRILL_SRC_DIR}/device/mpu"
    "${ATHRILL_SRC_DIR}/debugger/interaction"
    "${ATHRILL_SRC_DIR}/debugger/interaction/inc"
    "${ATHRILL_SRC_DIR}/debugger/executor"
    "${ATHRILL_SRC_DIR}/cpu"
    "${ATHRILL_SRC_DIR}/main"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config"
    "${ATHRILL_TARGET_SOURCE_DIR}/debugger/interaction/inc"
)
athrill_v850_configure_target(athrill_main)

add_library(athrill_cui STATIC ${ATHRILL_CUI_SOURCES})
target_include_directories(athrill_cui PRIVATE
    "${ATHRILL_SRC_DIR}/lib/dwarf"
    "${ATHRILL_SRC_DIR}/debugger/interaction"
    "${ATHRILL_SRC_DIR}/debugger/interaction/inc"
    "${ATHRILL_SRC_DIR}/debugger/executor"
    "${ATHRILL_SRC_DIR}/debugger/executor/concrete_executor/target"
    "${ATHRILL_SRC_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config"
    "${ATHRILL_TARGET_SOURCE_DIR}/debugger/interaction/inc"
)
athrill_v850_configure_target(athrill_cui)

add_library(athrill_v850_cpu STATIC
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_bit.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_branch.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_dbg.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_div.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_logic.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_arithm.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_store.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_load.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_spec.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_fpu.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec/op_exec_sat.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_dec/op_dec.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_dec/op_parse.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_dec/op_parse_private.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config/cpu_config.c"
    "${ATHRILL_SRC_DIR}/lib/dbg_log.c"
)
target_include_directories(athrill_v850_cpu PRIVATE
    "${ATHRILL_SRC_DIR}/cpu"
    "${ATHRILL_SRC_DIR}/bus"
    "${ATHRILL_SRC_DIR}/device/mpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_exec"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_dec"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/cpu_common"
    "${ATHRILL_TARGET_SOURCE_DIR}/device"
)
athrill_v850_configure_target(athrill_v850_cpu)

set(_athrill_device_sources
    ${ATHRILL_DEVICE_COMMON_SOURCES}
    ${ATHRILL_DEVICE_THREAD_SOURCES}
    "${ATHRILL_TARGET_SOURCE_DIR}/device/device.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/device/intc/intc.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/device/peripheral/timer/timer32.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/device/peripheral/serial/serial.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/debugger/executor/concrete_executor/dbg_target_serial.c"
    "${ATHRILL_TARGET_SOURCE_DIR}/debugger/executor/concrete_executor/dbg_target_cpu.c"
)
if(ATHRILL_ENABLE_EXDEV)
    list(APPEND _athrill_device_sources ${ATHRILL_DEVICE_EXDEV_SOURCES})
    if(NOT WIN32)
        list(APPEND _athrill_device_sources ${ATHRILL_DEVICE_EXDEV_POSIX_SOURCES})
    endif()
endif()
if(ATHRILL_ENABLE_MROS)
    list(APPEND _athrill_device_sources
        "${ATHRILL_TARGET_SOURCE_DIR}/device/peripheral/can/can.c")
endif()

add_library(athrill_v850_device STATIC ${_athrill_device_sources})
target_include_directories(athrill_v850_device PRIVATE
    "${ATHRILL_SRC_DIR}/cpu"
    "${ATHRILL_SRC_DIR}/device/mpu"
    "${ATHRILL_SRC_DIR}/debugger/executor"
    "${ATHRILL_SRC_DIR}/device/peripheral/target"
    "${ATHRILL_SRC_DIR}/device/peripheral/mros-dev/mros-athrill/api"
    "${ATHRILL_SRC_DIR}/device/peripheral/mros-dev/mros-athrill/device"
    "${ATHRILL_SRC_DIR}/device/peripheral/mros-dev/mros-athrill/target/os"
    "${ATHRILL_SRC_DIR}/device/peripheral/mros-dev/mros-src/os/target/os_asp"
    "${ATHRILL_SRC_DIR}/device/peripheral/serial/fifo"
    "${ATHRILL_APL_DIR}/include"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config"
    "${ATHRILL_TARGET_SOURCE_DIR}/device"
    "${ATHRILL_TARGET_SOURCE_DIR}/device/intc"
    "${ATHRILL_TARGET_SOURCE_DIR}/device/intc/gic"
    "${ATHRILL_TARGET_SOURCE_DIR}/device/intc/register"
    "${ATHRILL_TARGET_SOURCE_DIR}/device/peripheral"
    "${ATHRILL_TARGET_SOURCE_DIR}/device/peripheral/inc"
)
athrill_v850_configure_target(athrill_v850_device)

add_library(athrill_bus STATIC ${ATHRILL_BUS_SOURCES})
target_include_directories(athrill_bus PRIVATE
    "${ATHRILL_SRC_DIR}/device/mpu"
)
athrill_v850_configure_target(athrill_bus)

add_library(athrill_loader STATIC ${ATHRILL_LOADER_SOURCES})
target_include_directories(athrill_loader PRIVATE
    "${ATHRILL_SRC_DIR}/lib/dwarf"
    "${ATHRILL_SRC_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config"
)
athrill_v850_configure_target(athrill_loader)

add_library(athrill_mpu STATIC
    ${ATHRILL_MPU_SOURCES}
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config/mpu_config.c"
)
target_include_directories(athrill_mpu PRIVATE
    "${ATHRILL_SRC_DIR}/device/mpu"
    "${ATHRILL_SRC_DIR}/device/peripheral/serial/fifo"
    "${ATHRILL_SRC_DIR}/cpu"
    "${ATHRILL_SRC_DIR}/lib/dwarf"
    "${ATHRILL_SRC_DIR}/lib/dwarf/data_type"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config"
)
athrill_v850_configure_target(athrill_mpu)

set(_athrill_std_sources ${ATHRILL_STD_COMMON_SOURCES})
if(NOT WIN32)
    list(APPEND _athrill_std_sources ${ATHRILL_STD_POSIX_SOURCES})
endif()
add_library(athrill_std STATIC ${_athrill_std_sources})
athrill_v850_configure_target(athrill_std)

set(_athrill_optional_libraries)
if(ATHRILL_ENABLE_MROS)
    if(WIN32)
        message(FATAL_ERROR "ATHRILL_ENABLE_MROS is not supported on Windows")
    endif()
    add_library(athrill_mros STATIC ${ATHRILL_MROS_SOURCES})
    target_include_directories(athrill_mros PRIVATE
        "${ATHRILL_TARGET_SOURCE_DIR}/cpu"
        "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config"
        ${ATHRILL_MROS_INCLUDE_DIRS}
    )
    target_compile_definitions(athrill_mros PRIVATE
        TARGET_ATHRILL
        LWIP_TRANSPORT_ETHERNET
    )
    athrill_v850_configure_target(athrill_mros)
    list(APPEND _athrill_optional_libraries athrill_mros)
endif()

add_executable(athrill2 "${ATHRILL_SRC_DIR}/main/main.c")
target_include_directories(athrill2 PRIVATE
    "${ATHRILL_SRC_DIR}/lib/dwarf"
    "${ATHRILL_SRC_DIR}/bus"
    "${ATHRILL_SRC_DIR}/cpu"
    "${ATHRILL_SRC_DIR}/device/mpu"
    "${ATHRILL_SRC_DIR}/debugger/interaction"
    "${ATHRILL_SRC_DIR}/debugger/interaction/inc"
    "${ATHRILL_SRC_DIR}/debugger/executor"
    "${ATHRILL_SRC_DIR}/main"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu"
    "${ATHRILL_TARGET_SOURCE_DIR}/cpu/config"
    "${ATHRILL_TARGET_SOURCE_DIR}/debugger/interaction/inc"
)
athrill_v850_configure_target(athrill2)
target_link_libraries(athrill2 PRIVATE
    athrill_main
    athrill_cui
    athrill_v850_cpu
    athrill_v850_device
    athrill_bus
    athrill_loader
    athrill_mpu
    athrill_std
    ${_athrill_optional_libraries}
    athrill_host_platform
)

install(TARGETS athrill2
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(BUILD_TESTING)
    add_test(NAME athrill2-usage COMMAND athrill2)

    add_executable(athrill-argument-parser-test
        "test/argument_parser_test.c"
        "${ATHRILL_SRC_DIR}/main/option/argument_parser.c"
    )
    target_include_directories(athrill-argument-parser-test PRIVATE
        "${ATHRILL_SRC_DIR}/main/option"
    )
    athrill_apply_common_settings(athrill-argument-parser-test)
    add_test(NAME athrill-argument-parser COMMAND athrill-argument-parser-test)

    if(ATHRILL_ENABLE_EXDEV)
        add_library(athrill-exdev-test-device SHARED
            "test/exdev_test_device.c"
        )
        target_include_directories(athrill-exdev-test-device PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/test"
        )
        athrill_apply_common_settings(athrill-exdev-test-device)
        set_target_properties(athrill-exdev-test-device PROPERTIES
            PREFIX ""
        )

        add_executable(athrill-shared-library-test
            "test/shared_library_test.c"
            "${ATHRILL_SRC_DIR}/lib/shared_library.c"
        )
        target_include_directories(athrill-shared-library-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/test"
        )
        athrill_apply_common_settings(athrill-shared-library-test)
        target_link_libraries(athrill-shared-library-test PRIVATE
            ${CMAKE_DL_LIBS}
        )
        add_test(NAME athrill-exdev-shared-library
            COMMAND athrill-shared-library-test
                $<TARGET_FILE:athrill-exdev-test-device>
        )
    endif()
endif()

message(STATUS "Athrill source: ${ATHRILL_ROOT}")
message(STATUS "Athrill target: V850E2M")
message(STATUS "Athrill EXDEV: ${ATHRILL_ENABLE_EXDEV}")
message(STATUS "Athrill mROS: ${ATHRILL_ENABLE_MROS}")
message(STATUS "Athrill VDEV: ${ATHRILL_ENABLE_VDEV}")
