#!/usr/bin/env zsh

qa_test_health_matrix_run() {
  local mode
  local emulator
  local bpm
  local steps

  for mode in $QA_DISPLAY_MODES; do
    for emulator in $EMULATORS; do
      qa_send_display_mode "${emulator}" "${mode}"
      sleep 2
      for bpm in $QA_HEALTH_BPM_VALUES; do
        for steps in $QA_HEALTH_STEPS_VALUES; do
          qa_send_health_override "${emulator}" "${bpm}" "${steps}"
          sleep 2
        done
      done
    done
  done
}

qa_test_health_run() {
  log_info "Starting health automation."
  qa_test_health_matrix_run
}
