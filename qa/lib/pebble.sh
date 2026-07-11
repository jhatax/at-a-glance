#!/usr/bin/env zsh

qa_pebble_kill() {
  qa_run_command "pebble-kill" pebble kill --force
}

qa_pebble_wipe() {
  qa_run_command "pebble-wipe" pebble wipe
}

qa_pebble_clean() {
  qa_run_command "pebble-clean" pebble clean
}

qa_pebble_build_verbose() {
  local build_log_path
  build_log_path="$(qa_resolve_build_log_path)" || return 1
  qa_run_command_with_log "pebble-build-verbose" "${build_log_path}" pebble build -v
}

qa_pebble_install_emulator() {
  local emulator="$1"
  sleep 4
  qa_run_command "install-emulator-${emulator}" pebble install --emulator "${emulator}"
}

qa_pebble_install_phone() {
  local phone_ip="$1"
  qa_run_command "install-phone" pebble install --phone "${phone_ip}"
}

qa_pebble_set_battery_state() {
  local emulator="$1"
  local percent="$2"
  local charging_state="$3"

  if [[ "${charging_state}" == "1" ]]; then
    qa_run_command \
      "battery-${emulator}-${percent}-charging" \
      pebble emu-battery --emulator "${emulator}" --percent "${percent}" --charging
    return
  fi

  qa_run_command \
    "battery-${emulator}-${percent}-unplugged" \
    pebble emu-battery --emulator "${emulator}" --percent "${percent}"
}

qa_pebble_capture_screenshot() {
  local emulator="$1"
  local output_path="$2"

  qa_run_command_with_log \
    "screenshot-${emulator}" \
    "${QA_LOGS_DIR}/screenshot-$(qa_slugify "${output_path:t:r}").log" \
    pebble screenshot --emulator "${emulator}" --no-open "${output_path}"
}
