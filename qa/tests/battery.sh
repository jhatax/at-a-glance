#!/usr/bin/env zsh

qa_test_battery_run() {
  log_info "Starting battery automation."

  local emulator
  local mode
  local charging_state
  local level

  for emulator in $EMULATORS; do
    for mode in $QA_DISPLAY_MODES; do
      qa_send_display_mode "${emulator}" "${mode}"
      sleep 3
      for charging_state in $QA_BATTERY_CHARGING_STATES; do
        for level in $QA_BATTERY_LEVELS; do
          qa_pebble_set_battery_state "${emulator}" "${level}" "${charging_state}"
          sleep 2
        done
      done
    done
  done
}
