#!/usr/bin/env zsh

qa_test_smoke_run() {
  log_info "Starting smoke automation."

  qa_test_health_matrix_run
  sleep 4
  qa_test_weather_run $QA_SMOKE_WEATHER_CODES
  qa_test_battery_run
}
