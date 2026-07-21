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
  printf "Building the project.\n"
  run_command "pebble-build" python3 $PYTHON_RUNNER build
}

pebble_build_verbose() {
  printf "Running a verbose build for the project.\n"
  run_command "pebble-build-verbose" python3 $PYTHON_RUNNER build \
    --verbose --log-path "${DIRECT_BUILD_LOG_PATH}" || return
}

pebble_install_phone() {
  local phone_ip="$1"
  printf "Installing on phone with IP $phone_ip.\n"
  run_command "install-phone" pebble install --phone "${phone_ip}"
}
