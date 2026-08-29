if(NOT DEFINED QEMU OR NOT DEFINED ELF OR NOT DEFINED LOG)
  message(FATAL_ERROR "QEMU, ELF and LOG are required")
endif()

file(REMOVE "${LOG}")
execute_process(
  COMMAND "${QEMU}"
          -M none,memory-backend=mem
          -object memory-backend-ram,id=mem,size=1G
          -cpu cortex-r5
          -nographic -monitor none -serial none
          -device loader,file=${ELF},cpu-num=0
          -d in_asm,guest_errors -D "${LOG}"
  TIMEOUT 6
  RESULT_VARIABLE qemu_result
  OUTPUT_QUIET ERROR_QUIET)

if(NOT EXISTS "${LOG}")
  message(FATAL_ERROR "QEMU did not create an execution log: ${qemu_result}")
endif()
file(READ "${LOG}" qemu_log)
string(FIND "${qemu_log}" "IN: bb_firmware_main" main_position)
string(FIND "${qemu_log}" "Invalid read" invalid_read_position)
string(FIND "${qemu_log}" "Invalid write" invalid_write_position)
if(main_position EQUAL -1)
  message(FATAL_ERROR "Cortex-R5 did not reach bb_firmware_main: ${qemu_result}")
endif()
if(NOT invalid_read_position EQUAL -1 OR NOT invalid_write_position EQUAL -1)
  message(FATAL_ERROR "Cortex-R5 boot produced an invalid memory access")
endif()
message(STATUS "Cortex-R5 boot reached bb_firmware_main with MPU enabled")
