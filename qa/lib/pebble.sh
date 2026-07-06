#!/usr/bin/env zsh

qa_pebble_kill() {
  pebble kill --force
}

qa_pebble_wipe() {
  pebble wipe
}

qa_pebble_clean() {
  pebble clean
}

qa_pebble_build_verbose() {
  mkdir -p "${BUILD_LOG_PATH:h}"
  pebble build -v > "${BUILD_LOG_PATH}"
}

qa_pebble_install_emulator() {
  local emulator="$1"
  sleep 4
  pebble install --emulator "${emulator}"
}

qa_pebble_install_phone() {
  local phone_ip="$1"
  pebble install --phone "${phone_ip}"
}

qa_pebble_set_battery_state() {
  local emulator="$1"
  local percent="$2"
  local charging_state="$3"

  if [[ "${charging_state}" == "1" ]]; then
    pebble emu-battery --emulator "${emulator}" --percent "${percent}" \
      --charging
    return
  fi

  pebble emu-battery --emulator "${emulator}" --percent "${percent}"
}
