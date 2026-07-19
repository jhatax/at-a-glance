#!/usr/bin/env zsh

run_command_with_log() {
  local output_path="$1"
  shift

  local output_dir="${output_path:h}"

  mkdir -p "${output_dir}"
  "$@" > "${output_path}" 2>&1
}

run_command() {
  local label="$1"
  shift

  printf "Running Command: $label\n"
  "$@"
}

generate_compile_commands_db() {
  if [[ ! -f "${COMPILE_DB_SCRIPT}" ]]; then
    printf "Missing compile database generator: $COMPILE_DB_SCRIPT\n"
    return 0
  fi

  printf "Generating ${COMPILE_DB_PATH:t} from ${DIRECT_BUILD_LOG_PATH:t}\n"
  if run_command \
    "generate-compile-commands" \
    python3 "${COMPILE_DB_SCRIPT}" \
    --log-path "${DIRECT_BUILD_LOG_PATH}" \
    --output "${COMPILE_DB_PATH}" \
    --platform "${DEFAULT_PLATFORM}"; then
    return 0
  fi

  printf "Incremental build log had no compile commands; retrying with a clean build.\n"
  pebble_clean || return $?
  printf "Rebuilding At A Glance after compile-database fallback.\n"
  pebble_build_verbose || return $?
  printf "Clean rebuild completed successfully.\n"
  if run_command \
    "generate-compile-commands-retry" \
    python3 "${COMPILE_DB_SCRIPT}" \
    --log-path "${DIRECT_BUILD_LOG_PATH}" \
    --output "${COMPILE_DB_PATH}" \
    --platform "${DEFAULT_PLATFORM}"; then
    return 0
  fi

  return 0
}

execute_failure_mode() {
  # Iterate over COMMAND_ARGS to get failure_modes
  local mode=""

  for mode in $COMMAND_ARGS; do
    if [[ "${mode}" == "build-typo" ]]; then
      printf "Injecting deliberate build failure: pebble bulid -v.\n"
      run_command_with_log "${DIRECT_BUILD_LOG_PATH}" pebble bulid -v
      return $?
    fi
  done

  printf "Unsupported failure mode(s): $COMMAND_ARGS\n"
  return 1
}

install_on_emulators() {
  printf "Installing on selected emulators.\n"
  local emulator
  for emulator in $EMULATORS; do
    pebble_install_emulator "${emulator}"
  done
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
