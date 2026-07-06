#!/usr/bin/env zsh

qa_validate_csv_values() {
  local kind="$1"
  shift
  local -a values=("${(@P)1}")
  shift
  local -a supported=("$@")
  local value

  for value in $values; do
    array_contains "$value" $supported || \
    die "Unsupported ${kind} value '${value}'"
  done
}

qa_validate_config() {
  qa_validate_csv_values "emulator" EMULATORS $QA_SUPPORTED_EMULATORS

  if [[ "${RUN_QA}" == true ]]; then
    qa_validate_csv_values "test" TESTS $QA_SUPPORTED_TESTS
  fi

  if [[ "${PHONE_INSTALL}" == true && "${PHONE_IP}" == 169.254.* ]]; then
    die "${PHONE_IP} is link-local, not a usable LAN Developer Connection IP"
  fi
}
