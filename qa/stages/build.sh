#!/usr/bin/env zsh

qa_stage_build_project() {
  qa_stage_enter "build"

  if [[ "${BUILD}" != true ]]; then
    qa_stage_skip "build"
    return
  fi

  log_info "Building At A Glance."

  if [[ "${FAILURE_MODE}" == "build-typo" ]]; then
    local build_log_path
    build_log_path="$(qa_resolve_build_log_path)" || return 1
    log_info "Injecting deliberate build failure: pebble bulid -v."
    qa_run_command_with_log "pebble-build-verbose" "${build_log_path}" pebble bulid -v || return $?
    return 0
  fi

  qa_pebble_build_verbose || return $?
  log_info "Build completed successfully."
  QA_BUILD_EXECUTED=true
  BUILD=false
  qa_stage_pass "build"
}
