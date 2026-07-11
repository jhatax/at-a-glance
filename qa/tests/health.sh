#!/usr/bin/env zsh

qa_test_health_step_run() {
  qa_send_display_mode "${STEP_EMULATOR}" "${STEP_DISPLAY}"
  sleep 2
  qa_send_health_override "${STEP_EMULATOR}" "${STEP_HEALTH_BPM}" "${STEP_HEALTH_STEPS}"
  qa_maybe_capture_screenshot \
    "health" \
    "${STEP_ARTIFACT_IDENTITY}" \
    "${STEP_EMULATOR}" \
    "display-${STEP_DISPLAY}" \
    "bpm-${STEP_HEALTH_BPM}" \
    "steps-${STEP_HEALTH_STEPS}"
  sleep 2
}
