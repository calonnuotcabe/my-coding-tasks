execute_process(
  COMMAND "${MODEM}" --config "${CONFIG}" --events "${EVENTS}"
          --iq-out "${PREFIX}-one.iq" --trace "${PREFIX}-one.jsonl"
  RESULT_VARIABLE first)
execute_process(
  COMMAND "${MODEM}" --config "${CONFIG}" --events "${EVENTS}"
          --iq-out "${PREFIX}-two.iq" --trace "${PREFIX}-two.jsonl"
  RESULT_VARIABLE second)
if(NOT first EQUAL 0 OR NOT second EQUAL 0)
  message(FATAL_ERROR "deterministic runs did not pass")
endif()
file(SHA256 "${PREFIX}-one.iq" iq_one)
file(SHA256 "${PREFIX}-two.iq" iq_two)
file(SHA256 "${PREFIX}-one.jsonl" trace_one)
file(SHA256 "${PREFIX}-two.jsonl" trace_two)
if(NOT iq_one STREQUAL iq_two OR NOT trace_one STREQUAL trace_two)
  message(FATAL_ERROR "deterministic replay mismatch")
endif()

