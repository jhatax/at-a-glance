#!/usr/bin/env zsh

qa_test_weather_step_run() {
  qa_send_weather_message \
    "${STEP_EMULATOR}" \
    "${STEP_WEATHER_TEMP}" \
    "${STEP_WEATHER_CODE}" \
    "${STEP_WEATHER_IS_DAY}" \
    "${STEP_DISPLAY}"
  qa_maybe_capture_screenshot \
    "weather" \
    "${STEP_ARTIFACT_IDENTITY}" \
    "${STEP_EMULATOR}" \
    "display-${STEP_DISPLAY}" \
    "temp-${STEP_WEATHER_TEMP}" \
    "code-${STEP_WEATHER_CODE}" \
    "day-${STEP_WEATHER_IS_DAY}"
  sleep 1
}
