#!/usr/bin/env zsh

typeset -g QA_VALIDATION_ERROR_MESSAGE=""

qa_validate_fail() {
  QA_VALIDATION_ERROR_MESSAGE="$1"
  return 1
}

qa_validate_csv_values() {
  local kind="$1"
  shift
  local -a values=("${(@P)1}")
  shift
  local -a supported=("$@")
  local value

  for value in $values; do
    array_contains "$value" $supported || {
      qa_validate_fail "Unsupported ${kind} value '${value}'"
      return 1
    }
  done
}

qa_validate_config() {
  QA_VALIDATION_ERROR_MESSAGE=""
  qa_validate_csv_values "emulator" EMULATORS $QA_SUPPORTED_EMULATORS

  if [[ -n "${FAILURE_MODE}" ]]; then
    [[ "${FAILURE_MODE}" == "build-typo" ]] || {
      qa_validate_fail "Unsupported failure mode '${FAILURE_MODE}'"
      return 1
    }
    [[ "${BUILD}" == true ]] || {
      qa_validate_fail "--failure build-typo requires --build"
      return 1
    }
  fi

  if [[ -n "${SCENARIO_NAME}" && "${EMULATORS_EXPLICIT}" == true ]]; then
    qa_validate_fail "--scenario cannot be combined with --emulators"
    return 1
  fi

  if [[ -n "${SCENARIO_NAME}" && "${PHONE_INSTALL}" == true ]]; then
    qa_validate_fail "--scenario cannot be combined with --phone"
    return 1
  fi

  if [[ "${RUN_QA}" == true && -z "${SCENARIO_NAME}" ]]; then
    qa_validate_fail "--scenario is required for scenario-run QA execution"
    return 1
  fi

  if [[ "${PHONE_INSTALL}" == true && "${PHONE_IP}" == 169.254.* ]]; then
    qa_validate_fail "${PHONE_IP} is link-local, not a usable LAN Developer Connection IP"
    return 1
  fi
}
