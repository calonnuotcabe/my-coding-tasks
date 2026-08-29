cmake_minimum_required(VERSION 3.16)
project(modem101_exam01 C ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BB_ENABLE_SANITIZERS "Enable ASan and UBSan for host tests" OFF)

set(BB_WARNINGS
    -Wall -Wextra -Wconversion -Wshadow -Werror
    -Wstrict-prototypes -Wmissing-prototypes -Wformat=2)

add_library(modem_fw_host STATIC
    src/fw/runtime/runtime.c
    src/fw/memory/memory.c
    src/fw/codec/codec.c
    src/fw/phy/phy.c
    src/fw/channel/channel.c
    src/fw/l2/protocol.c
    src/fw/l2/stack.c
    src/fw/l2/mac.c
    src/fw/l2/rlc_um.c
    src/fw/l2/pdcp.c
    src/fw/security/security.c
    src/fw/ipc/ipc.c
    src/fw/control/transitions.c
    src/fw/control/state_machine.c
    src/hal/mmio.c
    src/fw/trace/trace.c
    src/soc/soc.c)
target_sources(modem_fw_host PRIVATE src/soc/dma_descriptor.c)
if(CMAKE_CROSSCOMPILING)
    target_sources(modem_fw_host PRIVATE arch/armv7r/test_key_arm.S)
else()
    target_sources(modem_fw_host PRIVATE src/soc/test_key_host.c)
endif()
target_include_directories(modem_fw_host PUBLIC include PRIVATE src/fw)
target_compile_options(modem_fw_host PRIVATE ${BB_WARNINGS} -ffreestanding -fno-builtin -fno-common)

if(NOT CMAKE_CROSSCOMPILING AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    target_sources(modem_fw_host PRIVATE arch/host/task_stack_x86_64.S)
    target_compile_definitions(modem_fw_host PRIVATE
        BB_HOST_TASK_STACK_SWITCH=1)
endif()

if(BB_ENABLE_SANITIZERS AND NOT CMAKE_CROSSCOMPILING)
    target_compile_options(modem_fw_host PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(modem_fw_host INTERFACE -fsanitize=address,undefined)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    add_executable(modem101_host src/host/main.c src/host/network_peer.c)
    set_target_properties(modem101_host PROPERTIES OUTPUT_NAME modem101)
    target_link_libraries(modem101_host PRIVATE modem_fw_host)
    target_compile_options(modem101_host PRIVATE ${BB_WARNINGS})

    enable_testing()
    add_executable(unit_q1 tests/unit_q1.c)
    target_link_libraries(unit_q1 PRIVATE modem_fw_host)
    target_compile_options(unit_q1 PRIVATE ${BB_WARNINGS})
    add_test(NAME unit_q1 COMMAND unit_q1)

    add_executable(unit_q2 tests/unit_q2.c)
    target_link_libraries(unit_q2 PRIVATE modem_fw_host)
    target_compile_options(unit_q2 PRIVATE ${BB_WARNINGS})
    add_test(NAME unit_q2 COMMAND unit_q2)

    add_executable(unit_runtime tests/unit_runtime.c)
    target_link_libraries(unit_runtime PRIVATE modem_fw_host)
    target_compile_options(unit_runtime PRIVATE ${BB_WARNINGS})
    add_test(NAME unit_runtime COMMAND unit_runtime)

    add_executable(unit_l2 tests/unit_l2.c)
    target_link_libraries(unit_l2 PRIVATE modem_fw_host)
    target_include_directories(unit_l2 PRIVATE src/fw)
    target_compile_options(unit_l2 PRIVATE ${BB_WARNINGS})
    add_test(NAME unit_l2 COMMAND unit_l2)

    add_executable(unit_defensive tests/unit_defensive.c)
    target_link_libraries(unit_defensive PRIVATE modem_fw_host)
    target_compile_options(unit_defensive PRIVATE ${BB_WARNINGS})
    add_test(NAME unit_defensive COMMAND unit_defensive)

    add_executable(unit_mac tests/unit_mac.c)
    target_link_libraries(unit_mac PRIVATE modem_fw_host)
    target_include_directories(unit_mac PRIVATE src/fw)
    target_compile_options(unit_mac PRIVATE ${BB_WARNINGS})
    add_test(NAME unit_mac COMMAND unit_mac)

    add_test(NAME integration_full
        COMMAND $<TARGET_FILE:modem101_host>
            --config ${CMAKE_CURRENT_SOURCE_DIR}/scenario.cfg
            --events ${CMAKE_CURRENT_SOURCE_DIR}/events.txt
            --iq-out ${CMAKE_CURRENT_BINARY_DIR}/integration-uplink.iq
            --trace ${CMAKE_CURRENT_BINARY_DIR}/integration-trace.jsonl)

    add_test(NAME reject_truncated_iq
        COMMAND ${CMAKE_COMMAND}
            -DMODEM=$<TARGET_FILE:modem101_host>
            -DCONFIG=${CMAKE_CURRENT_SOURCE_DIR}/scenario.cfg
            -DEVENTS=${CMAKE_CURRENT_SOURCE_DIR}/events.txt
            -DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/truncated.iq
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/bad_iq.cmake)

    add_test(NAME deterministic_replay
        COMMAND ${CMAKE_COMMAND}
            -DMODEM=$<TARGET_FILE:modem101_host>
            -DCONFIG=${CMAKE_CURRENT_SOURCE_DIR}/scenario.cfg
            -DEVENTS=${CMAKE_CURRENT_SOURCE_DIR}/events.txt
            -DPREFIX=${CMAKE_CURRENT_BINARY_DIR}/deterministic
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/deterministic.cmake)

    add_test(NAME integration_iq_input
        COMMAND ${CMAKE_COMMAND}
            -DMODEM=$<TARGET_FILE:modem101_host>
            -DCONFIG=${CMAKE_CURRENT_SOURCE_DIR}/scenario.cfg
            -DEVENTS=${CMAKE_CURRENT_SOURCE_DIR}/events.txt
            -DPREFIX=${CMAKE_CURRENT_BINARY_DIR}/iq-input
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/iq_input.cmake)
else()
    add_executable(bb_fw_arm
        arch/armv7r/startup_armv7r.S
        src/fw/arm_entry.c)
    target_link_libraries(bb_fw_arm PRIVATE modem_fw_host)
    target_link_libraries(bb_fw_arm PRIVATE gcc)
    target_compile_options(bb_fw_arm PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Wall>
        $<$<COMPILE_LANGUAGE:C>:-Wextra>
        $<$<COMPILE_LANGUAGE:C>:-Wconversion>
        $<$<COMPILE_LANGUAGE:C>:-Wshadow>
        $<$<COMPILE_LANGUAGE:C>:-Werror>
        $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>
        $<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>
        $<$<COMPILE_LANGUAGE:C>:-Wformat=2>
        $<$<COMPILE_LANGUAGE:C>:-ffreestanding>
        $<$<COMPILE_LANGUAGE:C>:-fno-builtin>
        $<$<COMPILE_LANGUAGE:C>:-fno-common>)
    target_link_options(bb_fw_arm PRIVATE
        -nostdlib
        -Wl,-z,noexecstack
        -Wl,--gc-sections
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/bb_fw_arm.map
        -T${CMAKE_CURRENT_SOURCE_DIR}/arch/armv7r/linker.ld)
    set_target_properties(bb_fw_arm PROPERTIES SUFFIX ".elf")
    find_program(BB_ARM_NM arm-none-eabi-nm REQUIRED)
    find_program(BB_ARM_READELF arm-none-eabi-readelf REQUIRED)
    add_custom_command(TARGET bb_fw_arm POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DNM=${BB_ARM_NM}
            -DREADELF=${BB_ARM_READELF}
            -DELF=$<TARGET_FILE:bb_fw_arm>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/arm_elf_audit.cmake
        VERBATIM)
    add_custom_target(arm_elf_audit DEPENDS bb_fw_arm)
    find_program(BB_QEMU_SYSTEM_ARM qemu-system-arm)
    if(BB_QEMU_SYSTEM_ARM)
        add_custom_target(arm_qemu_smoke
            COMMAND ${CMAKE_COMMAND}
                -DQEMU=${BB_QEMU_SYSTEM_ARM}
                -DELF=$<TARGET_FILE:bb_fw_arm>
                -DLOG=${CMAKE_CURRENT_BINARY_DIR}/arm-qemu-smoke.log
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/arm_qemu_smoke.cmake
            DEPENDS arm_elf_audit
            VERBATIM)
    endif()
endif()
