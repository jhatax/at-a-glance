#!/usr/bin/env zsh

typeset -g BUILD_LOG_PATH=""
typeset -g COMMAND_KIND=""
typeset -g COMMAND_ACTION=""
typeset -ga COMMAND_ARGS=()

typeset -g CLEAN=false
typeset -g BUILD=false
typeset -g INSTALL=false
typeset -g WIPE=false
typeset -g NUKE=false
typeset -g RUN_QA=false
typeset -g DRY_RUN=false
typeset -g FORCE=false
typeset -g PHONE_INSTALL=false
typeset -g PHONE_IP=""
typeset -g FAILURE_MODE=""
typeset -g EMULATORS_EXPLICIT=false
typeset -g PARSE_ERROR_MESSAGE=""
typeset -ga EMULATORS=(${QA_DEFAULT_EMULATORS[@]})

typeset -g SCENARIO_NAME=""
typeset -g QA_SCENARIO_STEPS_JSON_PATH=""
typeset -ga QA_STAGE_RESULTS=()
typeset -ga QA_TEST_RESULTS=()
typeset -ga QA_ASSERTION_RESULTS=()

typeset -g QA_EFFECTIVE_SCENARIO="adhoc"
typeset -g QA_CAPTURE_SCREENSHOTS="${QA_CAPTURE_SCREENSHOTS:-false}"

typeset -g QA_RUN_ID=""
typeset -g QA_ACTIVE_STAGE=""
typeset -g QA_ACTIVE_TEST=""
typeset -g QA_BUILD_EXECUTED=false
typeset -g QA_COMMAND_COUNT=0
typeset -g QA_SCREENSHOT_COUNT=0
typeset -g QA_FINALIZED=false
typeset -g QA_TRANSIENT_DIR=""

typeset -g QA_ARTIFACTS_ROOT=""
typeset -g QA_COMMANDS_LOG_PATH=""
typeset -g QA_RUN_JSON_PATH=""
typeset -g QA_REPORT_JSON_PATH=""
typeset -g QA_REPORT_MD_PATH=""
typeset -g QA_LOGS_DIR=""
typeset -g QA_SCREENSHOTS_DIR=""
typeset -g QA_SCREENSHOT_INDEX_PATH=""

typeset -g STEP_CAPABILITY=""
typeset -g STEP_ARTIFACT_IDENTITY=""
typeset -g STEP_EMULATOR=""
typeset -g STEP_DISPLAY=""
typeset -g STEP_WEATHER_TEMP=""
typeset -g STEP_WEATHER_CODE=""
typeset -g STEP_WEATHER_IS_DAY=""
typeset -g STEP_BATTERY_LEVEL=""
typeset -g STEP_BATTERY_CHARGING=""
typeset -g STEP_HEALTH_BPM=""
typeset -g STEP_HEALTH_STEPS=""

qa_clear_step_state() {
  STEP_CAPABILITY=""
  STEP_ARTIFACT_IDENTITY=""
  STEP_EMULATOR=""
  STEP_DISPLAY=""
  STEP_WEATHER_TEMP=""
  STEP_WEATHER_CODE=""
  STEP_WEATHER_IS_DAY=""
  STEP_BATTERY_LEVEL=""
  STEP_BATTERY_CHARGING=""
  STEP_HEALTH_BPM=""
  STEP_HEALTH_STEPS=""
}

qa_load_step_field() {
  local key="$1"
  local value="$2"

  case "${key}" in
    capability)
      STEP_CAPABILITY="${value}"
      ;;
    artifact_identity)
      STEP_ARTIFACT_IDENTITY="${value}"
      ;;
    emulator)
      STEP_EMULATOR="${value}"
      ;;
    display)
      STEP_DISPLAY="${value}"
      ;;
    temp)
      STEP_WEATHER_TEMP="${value}"
      ;;
    code)
      STEP_WEATHER_CODE="${value}"
      ;;
    is_day)
      STEP_WEATHER_IS_DAY="${value}"
      ;;
    level)
      STEP_BATTERY_LEVEL="${value}"
      ;;
    charging)
      STEP_BATTERY_CHARGING="${value}"
      ;;
    bpm)
      STEP_HEALTH_BPM="${value}"
      ;;
    steps)
      STEP_HEALTH_STEPS="${value}"
      ;;
    *)
      printf 'Error: Unknown step field %s\n' "${key}" >&2
      return 1
      ;;
  esac
}
