#!/usr/bin/env zsh

typeset -g QA_VALIDATION_ERROR_MESSAGE=""

command_is_qa_inspection() [[ "${COMMAND_TYPE}" == "qa-inspection" ]]

command_is_scenario_exec() [[ "${COMMAND_TYPE}" == "scenario-exec" ]]

command_is_env_prep() [[ "${COMMAND_TYPE}" == "env-prep" ]]

validate_fail() {
  QA_VALIDATION_ERROR_MESSAGE="$1"
  return 1
}

validate_csv_values() {
  local kind="$1"
  shift
  local -a values=("${(@P)1}")
  shift
  local -a supported=("$@")
  local value

  for value in $values; do
    [[ -n ${(M)supported:#$value} ]] || {
      validate_fail "Unsupported ${kind} value '${value}'"
      return
    }
  done
}

validate_config() {
  # one of the supported command types
  [[ "$COMMAND_TYPE" == (scenario-exec|env-prep|qa-inspection) ]] || {
    validate_fail "unsupported command requested"
    return
  }

  # an action is specified
  ((${#COMMAND_ACTION})) || {
    validate_fail "no action specified"
    return
  }

  # emulators are relevants only for install execution
  if [[ "$COMMAND_ACTION" == "install" ]]; then
    validate_csv_values "emulator" EMULATORS $ALL_EMULATORS || return
  fi

  # a scenario has specific command-actions
  if command_is_scenario_exec; then
    if [[ "${COMMAND_ACTION}" != (run|force)-scenario ]]; then
      validate_fail \
        "--qaplan must be combined with a scenario name and an optional --force flag"
      return
    fi

    if ((!${#COMMAND_ARGS[@]})); then
      validate_fail "no scenario specified to execute"
      return
    fi
  # env-prep and qa-inspection cannot use scenario-execution actions
  elif [[ "$COMMAND_ACTION" == (run|force)-scenario ]]; then
    validate_fail "--force was specified without --qaplan and name"
    return
  fi

  if command_is_env_prep; then
    if [[ "${COMMAND_ACTION}" == "phone" ]] && ((!${#COMMAND_ARGS[@]})); then
      validate_fail "no usable LAN Developer Connection IP specified for phone-install"
      return
    fi
  fi

  if command_is_qa_inspection; then
    if [[ "${COMMAND_ACTION}" != (runs|view-run|compare|validate|dryrun) ]]; then
      validate_fail "unsupported qa-inspection action '${COMMAND_ACTION}'"
      return
    fi

    if [[ "${COMMAND_ACTION}" == "validate" ]] && ((!${#COMMAND_ARGS[@]})); then
      validate_fail "no QA plan specified to validate"
      return
    fi

    if [[ "${COMMAND_ACTION}" == "dryrun" ]] && ((!${#COMMAND_ARGS[@]})); then
      validate_fail "no scenario specified for dryrun"
      return
    fi

    if [[ "${COMMAND_ACTION}" == "view-run" ]] && ((${#COMMAND_ARGS[@]} != 1)); then
      validate_fail "view-run requires exactly one run selector"
      return
    fi

    if [[ "${COMMAND_ACTION}" == "compare" ]] &&
      ((${#COMMAND_ARGS[@]} < 1 || ${#COMMAND_ARGS[@]} > 5)); then
      validate_fail "compare requires one to five run selectors"
      return
    fi
  fi
  # should automatically return 0 because of zsh convention
}
