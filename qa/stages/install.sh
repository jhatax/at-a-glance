#!/usr/bin/env zsh

qa_stage_install_selected_emulators() {
  if [[ "${INSTALL}" != true ]]; then
    return
  fi

  log_info "Installing on selected emulators."
  local emulator
  for emulator in $EMULATORS; do
    qa_pebble_install_emulator "${emulator}"
  done
  INSTALL=false
}

qa_stage_install_phone() {
  if [[ "${PHONE_INSTALL}" != true ]]; then
    return
  fi

  log_info "Installing through Pebble Developer Connection at ${PHONE_IP}."
  qa_pebble_install_phone "${PHONE_IP}"
  PHONE_INSTALL=false
}
