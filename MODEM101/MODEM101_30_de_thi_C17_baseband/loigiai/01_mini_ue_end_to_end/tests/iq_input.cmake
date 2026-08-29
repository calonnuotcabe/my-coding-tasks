execute_process(
  COMMAND "${MODEM}" --config "${CONFIG}" --events "${EVENTS}"
          --iq-out "${PREFIX}-seed.iq" --trace "${PREFIX}-seed.jsonl"
  RESULT_VARIABLE seed_result)
if(NOT seed_result EQUAL 0)
  message(FATAL_ERROR "could not generate valid IQ16 fixture")
endif()
execute_process(
  COMMAND "${MODEM}" --config "${CONFIG}" --events "${EVENTS}"
          --iq-in "${PREFIX}-seed.iq" --iq-out "${PREFIX}-output.iq"
          --trace "${PREFIX}-output.jsonl"
  RESULT_VARIABLE input_result)
if(NOT input_result EQUAL 0)
  message(FATAL_ERROR "valid IQ16 input path failed")
endif()

