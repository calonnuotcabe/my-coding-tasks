file(WRITE "${OUTPUT_FILE}" "IQ16")
execute_process(
  COMMAND "${MODEM}" --config "${CONFIG}" --events "${EVENTS}"
          --iq-in "${OUTPUT_FILE}" --iq-out "${OUTPUT_FILE}.out"
          --trace "${OUTPUT_FILE}.jsonl"
  RESULT_VARIABLE result)
if(result EQUAL 0)
  message(FATAL_ERROR "truncated IQ16 input was accepted")
endif()

