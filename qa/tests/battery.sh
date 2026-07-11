#!/usr/bin/env zsh

qa_test_battery_step_run() {
  qa_send_display_mode "${STEP_EMULATOR}" "${STEP_DISPLAY}"
  sleep 3
  qa_pebble_set_battery_state "${STEP_EMULATOR}" "${STEP_BATTERY_LEVEL}" "${STEP_BATTERY_CHARGING}"
  qa_maybe_capture_screenshot \
    "battery" \
    "${STEP_ARTIFACT_IDENTITY}" \
    "${STEP_EMULATOR}" \
    "display-${STEP_DISPLAY}" \
    "level-${STEP_BATTERY_LEVEL}" \
    "charging-${STEP_BATTERY_CHARGING}"
  sleep 2
}
