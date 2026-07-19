#!/usr/bin/env zsh

handle_env_prep() {
  command_is_env_prep || return 0
  case "$COMMAND_ACTION" in
    help)
      print_help
      ;;
    failure)
      execute_failure_mode
      ;;
    nuke)
      printf "Running nuclear reset: wipe emulator state and force a clean rebuild.\n"
      pebble_kill || return
      execute_next_pebble_action pebble_wipe || return
      execute_next_pebble_action pebble_clean || return
      execute_next_pebble_action pebble_build_verbose || return
      ;;
    wipe)
      pebble_wipe
      ;;
    build-clean)
      pebble_clean || return
      execute_next_pebble_action pebble_build_verbose
      ;;
    build)
      pebble_build
      ;;
    build-verbose)
      pebble_build_verbose
      ;;
    install)
      install_on_emulators
      ;;
    phone)
      install_on_phone
      ;;
  esac
}

# Confirm qa plans, view details about test runs
handle_qa_inspections() {
  command_is_qa_inspection || return 0
  python3 "${QA_DIR}/runner.py" qa-inspection "${COMMAND_ACTION}" "${COMMAND_ARGS[@]}"
}

# Execute the selected QA plan
handle_qa_plan_execution() {
  command_is_scenario_exec || return 0
  python3 "${QA_DIR}/runner.py" scenario-exec "$COMMAND_ACTION" --qaplan-name "$COMMAND_ARGS[1]"
}
