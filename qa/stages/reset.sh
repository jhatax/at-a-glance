#!/usr/bin/env zsh

qa_stage_prepare_reset_actions() {
  qa_stage_enter "reset"

  log_info "Resetting Pebble emulator state for this run."
  qa_pebble_kill || return $?

  if [[ "${NUKE}" == true ]]; then
    log_info "Running nuclear reset: wipe emulator state and force a clean rebuild."
    qa_pebble_wipe || return $?
    qa_pebble_clean || return $?
    WIPE=false
    CLEAN=false
    BUILD=true
    NUKE=false
    qa_stage_pass "reset"
    return
  fi

  if [[ "${WIPE}" == true ]]; then
    log_info "Wiping emulator state."
    qa_pebble_wipe || return $?
    WIPE=false
  fi

  if [[ "${CLEAN}" == true ]]; then
    log_info "Cleaning Pebble build artifacts."
    qa_pebble_clean || return $?
    CLEAN=false
    BUILD=true
  fi

  qa_stage_pass "reset"
}

qa_stage_cleanup_actions() {
  qa_stage_enter "cleanup"

  log_info "Cleaning up Pebble emulator state after this run."
  qa_pebble_wipe || return $?
  qa_pebble_kill || return $?

  qa_stage_pass "cleanup"
}
