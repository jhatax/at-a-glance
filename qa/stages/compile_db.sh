#!/usr/bin/env zsh

qa_stage_generate_compile_database() {
  [[ -f "${COMPILE_DB_SCRIPT}" ]] || \
    die "Missing compile database generator: ${COMPILE_DB_SCRIPT}"

  log_info "Generating ${COMPILE_DB_PATH:t} from ${BUILD_LOG_PATH:t}."
  python3 "${COMPILE_DB_SCRIPT}" \
    --log-path "${BUILD_LOG_PATH}" \
    --output "${COMPILE_DB_PATH}" \
    --platform "${DEFAULT_PLATFORM}"
}
