#!/usr/bin/env zsh

qa_stage_install_targets() {
  qa_stage_enter "install"

  if [[ "${INSTALL}" != true && "${PHONE_INSTALL}" != true ]]; then
    qa_stage_skip "install"
    return
  fi

  if [[ "${INSTALL}" == true ]]; then
    log_info "Installing on selected emulators."
    local emulator
    for emulator in $EMULATORS; do
      qa_pebble_install_emulator "${emulator}"
    done
    INSTALL=false
  fi

  if [[ "${PHONE_INSTALL}" == true ]]; then
    log_info "Installing through Pebble Developer Connection at ${PHONE_IP}."
    qa_pebble_install_phone "${PHONE_IP}"
    PHONE_INSTALL=false
  fi

  qa_stage_pass "install"
}
