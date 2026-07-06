#!/usr/bin/env zsh

qa_test_weather_run() {
  log_info "Starting weather automation."

  local -a weather_codes=("$@")
  local mode
  local day_state
  local weather_code
  local emulator

  # If there is more than one element in the array, continue, else
  # set the array to be the default set of weather codes
  (( $#weather_codes )) || weather_codes=($QA_WEATHER_CODES)

  for mode in $QA_DISPLAY_MODES; do
    for day_state in $QA_DAY_STATES; do
      for weather_code in $weather_codes; do
        for emulator in $EMULATORS; do
          qa_send_weather_message \
            "${emulator}" \
            "${QA_WEATHER_TEST_TEMPERATURE}" \
            "${weather_code}" \
            "${day_state}" \
            "${mode}"
          sleep 1
        done
      done
    done
  done
}
