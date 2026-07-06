#!/usr/bin/env zsh

qa_stage_prepare_reset_actions() {
  if [[ "${NUKE}" == true ]]; then
    log_info "Killing emulators and preparing a clean rebuild."
    qa_pebble_kill
    WIPE=true
    CLEAN=true
  fi

  if [[ "${WIPE}" == true ]]; then
    log_info "Wiping emulator state."
    qa_pebble_wipe
    WIPE=false
  fi

  if [[ "${CLEAN}" == true ]]; then
    log_info "Cleaning Pebble build artifacts."
    qa_pebble_clean
    CLEAN=false
    BUILD=true
  fi
}
