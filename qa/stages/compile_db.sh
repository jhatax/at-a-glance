#!/usr/bin/env zsh

qa_stage_generate_compile_database() {
  qa_stage_enter "compile-database"

  if [[ "${QA_BUILD_EXECUTED}" != true ]]; then
    qa_stage_skip "compile-database"
    return
  fi

  if [[ ! -f "${COMPILE_DB_SCRIPT}" ]]; then
    log_info "Missing compile database generator: ${COMPILE_DB_SCRIPT}"
    qa_stage_fail_active
    return 0
  fi

  log_info "Generating ${COMPILE_DB_PATH:t} from ${BUILD_LOG_PATH:t}."
  if qa_run_command \
    "generate-compile-commands" \
    python3 "${COMPILE_DB_SCRIPT}" \
    --log-path "${BUILD_LOG_PATH}" \
    --output "${COMPILE_DB_PATH}" \
    --platform "${DEFAULT_PLATFORM}"; then
    qa_stage_pass "compile-database"
    return
  fi

  log_info "Incremental build log had no compile commands; retrying with a clean build."
  qa_pebble_clean
  log_info "Rebuilding At A Glance after compile-database fallback."
  qa_pebble_build_verbose
  log_info "Clean rebuild completed successfully."
  if qa_run_command \
    "generate-compile-commands-retry" \
    python3 "${COMPILE_DB_SCRIPT}" \
    --log-path "${BUILD_LOG_PATH}" \
    --output "${COMPILE_DB_PATH}" \
    --platform "${DEFAULT_PLATFORM}"; then
    qa_stage_pass "compile-database"
    return
  fi

  qa_stage_fail_active
  return 0
}
