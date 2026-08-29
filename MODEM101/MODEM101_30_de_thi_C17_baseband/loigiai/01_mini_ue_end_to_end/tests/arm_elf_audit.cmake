if(NOT DEFINED NM OR NOT DEFINED READELF OR NOT DEFINED ELF)
  message(FATAL_ERROR "NM, READELF and ELF are required")
endif()

execute_process(
  COMMAND "${NM}" -u "${ELF}"
  RESULT_VARIABLE nm_result
  OUTPUT_VARIABLE unresolved
  ERROR_VARIABLE nm_error)
string(STRIP "${unresolved}" unresolved)
if(NOT nm_result EQUAL 0 OR NOT unresolved STREQUAL "")
  message(FATAL_ERROR "ARM ELF has unresolved symbols: ${unresolved}${nm_error}")
endif()

execute_process(
  COMMAND "${READELF}" -hW "${ELF}"
  RESULT_VARIABLE header_result
  OUTPUT_VARIABLE elf_header)
if(NOT header_result EQUAL 0 OR
   NOT elf_header MATCHES "Class:[ ]+ELF32" OR
   NOT elf_header MATCHES "Data:[ ]+2's complement, little endian" OR
   NOT elf_header MATCHES "Machine:[ ]+ARM" OR
   NOT elf_header MATCHES "soft-float ABI")
  message(FATAL_ERROR "ARM ELF header/ABI audit failed")
endif()

execute_process(
  COMMAND "${READELF}" -lW "${ELF}"
  RESULT_VARIABLE program_result
  OUTPUT_VARIABLE program_headers)
if(NOT program_result EQUAL 0)
  message(FATAL_ERROR "could not inspect ARM ELF program headers")
endif()
string(REGEX MATCH "LOAD[^\n]*W[^\n]*E|LOAD[^\n]*E[^\n]*W"
       writable_executable "${program_headers}")
if(NOT writable_executable STREQUAL "")
  message(FATAL_ERROR "ARM ELF contains a writable executable LOAD segment")
endif()

execute_process(
  COMMAND "${READELF}" -SW "${ELF}"
  RESULT_VARIABLE section_result
  OUTPUT_VARIABLE sections)
if(NOT section_result EQUAL 0 OR
   NOT sections MATCHES "\\.ARM\\.exidx[^\n]*001[0-3]")
  message(FATAL_ERROR ".ARM.exidx is missing or outside TCM_RX")
endif()

message(STATUS "ARM ELF audit passed: ELF32 little-endian soft-float, no undefined or W+X")
