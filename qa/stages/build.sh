#!/usr/bin/env zsh

qa_stage_build_project() {
  if [[ "${BUILD}" != true ]]; then
    return
  fi

  log_info "Building At A Glance."
  qa_pebble_build_verbose
  log_info "Build completed successfully."

  if qa_stage_generate_compile_database; then
    return
  fi

  log_info "Incremental build log had no compile commands; retrying with a clean build."
  qa_pebble_clean
  qa_pebble_build_verbose
  log_info "Clean rebuild completed successfully."
  qa_stage_generate_compile_database
  BUILD=false
}
