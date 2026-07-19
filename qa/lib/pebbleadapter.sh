#!/usr/bin/env zsh

pebble_kill() {
  printf "Terminating all active emulators.\n"
  run_command "pebble-kill" pebble kill --force
}

pebble_wipe() {
  printf "Wiping emulator state.\n"
  run_command "pebble-wipe" pebble wipe
}

pebble_clean() {
  printf "Cleaning Pebble build artifacts.\n"
  run_command "pebble-clean" pebble clean
}

pebble_build() {
  run_command "pebble-build" pebble build
}

pebble_build_verbose() {
  run_command_with_log "${DIRECT_BUILD_LOG_PATH}" pebble build -v || return
  execute_next_pebble_action generate_compile_commands_db
}

pebble_install_emulator() {
  local emulator="$1"
  sleep 4
  run_command "install-emulator-${emulator}" pebble install --emulator "${emulator}"
}

pebble_install_phone() {
  local phone_ip="$1"
  run_command "install-phone" pebble install --phone "${phone_ip}"
}

execute_next_pebble_action() {
  printf 'Running Command: %q\n' "$@"
  if command_is_env_prep; then
    "$@"
  fi
}
