#!/usr/bin/env zsh

run_command() {
  local label="$1"
  shift

  "$@"
}

install_on_phone() {
  local ip=""

  for ip in $COMMAND_ARGS; do
    if [[ -n "${ip}" ]]; then
      printf "Installing through Pebble Developer Connection at $ip.\n"
      pebble_install_phone "${ip}"
    fi
  done
}

# Get ready for QA plan execution
prepare_reset_actions() {
  printf "Resetting Pebble emulator state for this run.\n"
  pebble_kill || return $?
  pebble_wipe || return $?
}

# Clean up after QA plan execution
cleanup_actions() {
  printf "Cleaning up Pebble emulator state after this run.\n"
  pebble_wipe || return $?
  pebble_kill || return $?
}
