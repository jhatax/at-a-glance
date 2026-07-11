#!/usr/bin/env zsh

qa_timestamp_utc() {
  TZ=UTC date +"%Y-%m-%dT%H:%M:%SZ"
}

qa_slugify() {
  local value="$1"
  value="$(print -r -- "${value}" | tr '[:upper:]' '[:lower:]')"
  value="$(print -r -- "${value}" | sed -E 's/[^a-z0-9]+/-/g; s/^-+//; s/-+$//')"

  if [[ -z "${value}" ]]; then
    printf 'item\n'
    return
  fi

  printf '%s\n' "${value}"
}

qa_result_exists() {
  local target_name="$1"
  local -a records=("${(@P)2}")
  local record

  for record in "${records[@]}"; do
    if [[ "${record%%|*}" == "${target_name}" ]]; then
      return 0
    fi
  done

  return 1
}

qa_stage_enter() {
  QA_ACTIVE_STAGE="$1"
}

qa_stage_pass() {
  QA_STAGE_RESULTS+=("$1|passed")
  QA_ACTIVE_STAGE=""
}

qa_stage_skip() {
  QA_STAGE_RESULTS+=("$1|skipped")
  QA_ACTIVE_STAGE=""
}

qa_stage_fail_active() {
  if [[ -z "${QA_ACTIVE_STAGE}" ]]; then
    return
  fi

  if ! qa_result_exists "${QA_ACTIVE_STAGE}" QA_STAGE_RESULTS; then
    QA_STAGE_RESULTS+=("${QA_ACTIVE_STAGE}|failed")
  fi
}

qa_test_enter() {
  QA_ACTIVE_TEST="$1"
}

qa_test_pass() {
  QA_TEST_RESULTS+=("$1|passed")
  QA_ACTIVE_TEST=""
}

qa_test_fail_active() {
  if [[ -z "${QA_ACTIVE_TEST}" ]]; then
    return
  fi

  if ! qa_result_exists "${QA_ACTIVE_TEST}" QA_TEST_RESULTS; then
    QA_TEST_RESULTS+=("${QA_ACTIVE_TEST}|failed")
  fi
}

qa_assert_record() {
  local name="$1"
  local result="$2"
  local detail="${3:-}"

  QA_ASSERTION_RESULTS+=("${name}|${result}|${detail}")
}

qa_set_array_from_csv() {
  local csv="$1"
  local array_name="$2"
  local -a values=()

  if [[ -n "${csv}" ]]; then
    values=("${(@s:,:)csv}")
  fi

  case "${array_name}" in
    EMULATORS)
      EMULATORS=("${values[@]}")
      ;;
    *)
      printf 'Error: Unsupported bootstrap array %s\n' "${array_name}" >&2
      return 1
      ;;
  esac
}

qa_apply_bootstrap_record() {
  local key="$1"
  local value="$2"

  case "${key}" in
    QA_RUN_ID)
      QA_RUN_ID="${value}"
      ;;
    QA_EFFECTIVE_SCENARIO)
      QA_EFFECTIVE_SCENARIO="${value}"
      ;;
    QA_CAPTURE_SCREENSHOTS)
      QA_CAPTURE_SCREENSHOTS="${value}"
      ;;
    QA_ARTIFACTS_ROOT)
      QA_ARTIFACTS_ROOT="${value}"
      ;;
    QA_COMMANDS_LOG_PATH)
      QA_COMMANDS_LOG_PATH="${value}"
      ;;
    QA_RUN_JSON_PATH)
      QA_RUN_JSON_PATH="${value}"
      ;;
    QA_REPORT_JSON_PATH)
      QA_REPORT_JSON_PATH="${value}"
      ;;
    QA_REPORT_MD_PATH)
      QA_REPORT_MD_PATH="${value}"
      ;;
    QA_LOGS_DIR)
      QA_LOGS_DIR="${value}"
      ;;
    QA_SCREENSHOTS_DIR)
      QA_SCREENSHOTS_DIR="${value}"
      ;;
    QA_SCREENSHOT_INDEX_PATH)
      QA_SCREENSHOT_INDEX_PATH="${value}"
      ;;
    QA_SCENARIO_STEPS_JSON_PATH)
      QA_SCENARIO_STEPS_JSON_PATH="${value}"
      ;;
    BUILD_LOG_PATH)
      BUILD_LOG_PATH="${value}"
      ;;
    RUN_QA)
      RUN_QA="${value}"
      ;;
    INSTALL)
      INSTALL="${value}"
      ;;
    PHONE_INSTALL)
      PHONE_INSTALL="${value}"
      ;;
    PHONE_IP)
      PHONE_IP="${value}"
      ;;
    EMULATORS_CSV)
      [[ -n "${value}" ]] && qa_set_array_from_csv "${value}" EMULATORS
      ;;
    "")
      ;;
    *)
      printf 'Error: Unknown bootstrap key %s\n' "${key}" >&2
      return 1
      ;;
  esac
}

qa_python_bootstrap() {
  local key
  local value

  while IFS=$'\t' read -r key value; do
    [[ -n "${key}" ]] || continue
    qa_apply_bootstrap_record "${key}" "${value}" || return 1
  done < <(
    env \
    SCRIPT_DIR="${SCRIPT_DIR}" \
    QA_DIR="${QA_DIR}" \
    COMPILE_DB_PATH="${COMPILE_DB_PATH}" \
    BUILD="${BUILD}" \
    CLEAN="${CLEAN}" \
    INSTALL="${INSTALL}" \
    WIPE="${WIPE}" \
    NUKE="${NUKE}" \
    RUN_QA="${RUN_QA}" \
    PHONE_INSTALL="${PHONE_INSTALL}" \
    PHONE_IP="${PHONE_IP}" \
    SCENARIO_NAME="${SCENARIO_NAME}" \
    EMULATORS_CSV="${(j:,:)EMULATORS}" \
    python3 "${QA_DIR}/runner.py" bootstrap
  ) || return
}

qa_is_direct_command() {
  [[ "${COMMAND_KIND}" == "direct" ]]
}

qa_transient_dir() {
  if [[ -n "${QA_TRANSIENT_DIR}" ]]; then
    printf '%s\n' "${QA_TRANSIENT_DIR}"
    return 0
  fi

  QA_TRANSIENT_DIR="$(mktemp -d "${TMPDIR:-/tmp}/aag-build-qa.XXXXXX")" || return 1
  printf '%s\n' "${QA_TRANSIENT_DIR}"
}

qa_resolve_build_log_path() {
  if [[ -n "${BUILD_LOG_PATH}" ]]; then
    printf '%s\n' "${BUILD_LOG_PATH}"
    return 0
  fi

  BUILD_LOG_PATH="$(qa_transient_dir)/build.log" || return 1
  printf '%s\n' "${BUILD_LOG_PATH}"
}

qa_log_command_line() {
  local command_status="$1"
  local label="$2"
  local log_path="$3"
  shift 3

  if [[ -z "${QA_COMMANDS_LOG_PATH}" ]]; then
    return 0
  fi

  local line
  line="$(printf '[%s] status=%s label=%s log=%s cmd=' \
    "$(qa_timestamp_utc)" \
    "${command_status}" \
    "${label}" \
    "${log_path}")"
  line="${line}$(printf '%q ' "$@")"$'\n'
  mkdir -p "${QA_COMMANDS_LOG_PATH:h}"
  printf '%s' "${line}" >> "${QA_COMMANDS_LOG_PATH}"
}

qa_run_command_with_log() {
  local label="$1"
  local output_path="$2"
  shift 2

  local output_dir="${output_path:h}"
  local command_status=0

  if qa_is_direct_command; then
    if [[ -n "${output_path}" ]]; then
      mkdir -p "${output_dir}"
      set +e
      "$@" 2>&1 | tee "${output_path}"
      command_status=${pipestatus[1]}
      set -e
      return "${command_status}"
    fi

    "$@"
    return $?
  fi

  mkdir -p "${output_dir}"
  QA_COMMAND_COUNT=$(( QA_COMMAND_COUNT + 1 ))
  qa_log_command_line "started" "${label}" "${output_path}" "$@"

  set +e
  "$@" > "${output_path}" 2>&1
  command_status=$?
  set -e

  if (( command_status != 0 )); then
    qa_log_command_line "failed(${command_status})" "${label}" "${output_path}" "$@"
    printf 'Command failed: %s (see %s)\n' "${label}" "${output_path}" >&2
    return "${command_status}"
  fi

  qa_log_command_line "passed" "${label}" "${output_path}" "$@"
}

qa_run_command() {
  local label="$1"
  shift

  if qa_is_direct_command; then
    "$@"
    return $?
  fi

  local slug
  slug="$(qa_slugify "${label}")"
  local next_count=$(( QA_COMMAND_COUNT + 1 ))
  local output_path
  output_path="${QA_LOGS_DIR}/$(printf '%03d' "${next_count}")-${slug}.log"

  qa_run_command_with_log "${label}" "${output_path}" "$@"
}

qa_join_tags() {
  local -a tags=("$@")
  printf '%s' "${(j:,:)tags}"
}

qa_capture_screenshot() {
  local capability="$1"
  local artifact_identity="$2"
  local emulator="$3"
  shift 3

  local -a state_tags=("$@")
  local filename="${artifact_identity}.png"

  local output_path="${QA_SCREENSHOTS_DIR}/${filename}"
  qa_pebble_capture_screenshot "${emulator}" "${output_path}"
  QA_SCREENSHOT_COUNT=$(( QA_SCREENSHOT_COUNT + 1 ))

  printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
    "${QA_EFFECTIVE_SCENARIO}" \
    "${capability}" \
    "${artifact_identity}" \
    "${emulator}" \
    "$(qa_join_tags "${state_tags[@]}")" \
    "screenshots/${filename}" >> "${QA_SCREENSHOT_INDEX_PATH}"
}

qa_maybe_capture_screenshot() {
  if [[ "${QA_CAPTURE_SCREENSHOTS}" != true ]]; then
    return
  fi

  qa_capture_screenshot "$@"
}

qa_load_step_from_record() {
  local record="$1"
  local pair
  local key
  local value

  qa_clear_step_state

  for pair in "${(@s:\t:)record}"; do
    [[ -n "${pair}" ]] || continue
    key="${pair%%=*}"
    value="${pair#*=}"
    [[ "${key}" != "${pair}" ]] || {
      printf 'Error: Invalid step record field %s\n' "${pair}" >&2
      return 1
    }
    qa_load_step_field "${key}" "${value}" || return 1
  done

  if [[ -z "${STEP_CAPABILITY}" || -z "${STEP_ARTIFACT_IDENTITY}" || -z "${STEP_EMULATOR}" ]]; then
    printf 'Error: Incomplete step record\n' >&2
    return 1
  fi
}

qa_run_selected_scenario() {
  local record

  while IFS= read -r record; do
    [[ -n "${record}" ]] || continue
    qa_load_step_from_record "${record}" || return 1
    qa_test_enter "${STEP_ARTIFACT_IDENTITY}"

    case "${STEP_CAPABILITY}" in
      weather)
        qa_test_weather_step_run
        ;;
      battery)
        qa_test_battery_step_run
        ;;
      health)
        qa_test_health_step_run
        ;;
      *)
        printf 'Error: Unsupported step capability %s\n' "${STEP_CAPABILITY}" >&2
        return 1
        ;;
    esac

    qa_test_pass "${STEP_ARTIFACT_IDENTITY}"
  done < <(python3 "${QA_DIR}/runner.py" emit-steps "${QA_SCENARIO_STEPS_JSON_PATH}")
}

qa_runtime_finalize() {
  local exit_status="$1"
  local finalize_status=0

  if [[ "${QA_FINALIZED}" == true || -z "${QA_RUN_ID}" || -z "${QA_RUN_JSON_PATH}" ]]; then
    return "${exit_status}"
  fi

  if [[ ! -f "${QA_RUN_JSON_PATH}" ]]; then
    return "${exit_status}"
  fi

  QA_FINALIZED=true

  qa_stage_fail_active
  qa_test_fail_active

  if ! qa_result_exists "artifact-capture" QA_STAGE_RESULTS; then
    QA_STAGE_RESULTS+=("artifact-capture|passed")
  fi
  if ! qa_result_exists "report-emission" QA_STAGE_RESULTS; then
    QA_STAGE_RESULTS+=("report-emission|passed")
  fi

  if (( exit_status == 0 )); then
    qa_assert_record "run-exit-status" "passed" "exit status was 0"
  else
    qa_assert_record "run-exit-status" "failed" "exit status was ${exit_status}"
  fi

  env \
  QA_RUN_JSON_PATH="${QA_RUN_JSON_PATH}" \
  QA_REPORT_JSON_PATH="${QA_REPORT_JSON_PATH}" \
  QA_REPORT_MD_PATH="${QA_REPORT_MD_PATH}" \
  QA_EFFECTIVE_SCENARIO="${QA_EFFECTIVE_SCENARIO}" \
  QA_ACTIVE_STAGE="${QA_ACTIVE_STAGE}" \
  QA_ACTIVE_TEST="${QA_ACTIVE_TEST}" \
  QA_COMMAND_COUNT="${QA_COMMAND_COUNT}" \
  QA_SCREENSHOT_COUNT="${QA_SCREENSHOT_COUNT}" \
  QA_ARTIFACTS_ROOT="${QA_ARTIFACTS_ROOT}" \
  QA_COMMANDS_LOG_PATH="${QA_COMMANDS_LOG_PATH}" \
  QA_SCREENSHOTS_DIR="${QA_SCREENSHOTS_DIR}" \
  QA_SCREENSHOT_INDEX_PATH="${QA_SCREENSHOT_INDEX_PATH}" \
  QA_LOGS_DIR="${QA_LOGS_DIR}" \
  BUILD_LOG_PATH="${BUILD_LOG_PATH}" \
  COMPILE_DB_PATH="${COMPILE_DB_PATH}" \
  QA_EXIT_STATUS="${exit_status}" \
  QA_STAGE_RESULTS_LINES="${(F)QA_STAGE_RESULTS}" \
  QA_TEST_RESULTS_LINES="${(F)QA_TEST_RESULTS}" \
  QA_ASSERTION_RESULTS_LINES="${(F)QA_ASSERTION_RESULTS}" \
  python3 "${QA_DIR}/runner.py" finalize >/dev/null || finalize_status=$?

  if (( finalize_status > 1 )); then
    printf 'Error: Python finalizer failed.\n' >&2
  fi

  if (( exit_status == 0 && finalize_status != 0 )); then
    return "${finalize_status}"
  fi

  return "${exit_status}"
}

qa_print_run_closeout() {
  local report_status="failed"
  local report_scenario=""
  local report_run_id=""
  local step_passed=0
  local step_failed=0
  local assertion_passed=0
  local assertion_failed=0
  local screenshots_expected=0
  local screenshots_captured=0
  local next_action=""

  if [[ -f "${QA_REPORT_JSON_PATH}" ]]; then
    IFS=$'\t' read -r report_status report_scenario report_run_id step_passed step_failed assertion_passed assertion_failed screenshots_expected screenshots_captured next_action < <(
      python3 - <<'PY' "${QA_REPORT_JSON_PATH}"
import json
import sys
from pathlib import Path

payload = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
run = payload.get("run", {})
summary = payload.get("summary", {})
steps = payload.get("steps", {})
assertions = payload.get("assertions", [])
operator_actions = payload.get("operator_actions", [])

step_passed = sum(1 for step in steps.values() if step.get("status") == "passed")
step_failed = sum(1 for step in steps.values() if step.get("status") == "failed")
assertion_passed = sum(1 for assertion in assertions if assertion.get("status") == "passed")
assertion_failed = sum(1 for assertion in assertions if assertion.get("status") == "failed")
screenshots_expected = summary.get("screenshots", 0)
screenshots_captured = sum(step.get("screenshot", {}).get("captured", 0) for step in steps.values())

print(
  "\t".join(
    [
      payload.get("status", "failed"),
      run.get("scenario", ""),
      run.get("run_id", ""),
      str(step_passed),
      str(step_failed),
      str(assertion_passed),
      str(assertion_failed),
      str(screenshots_expected),
      str(screenshots_captured),
      operator_actions[0].get("label", "") if operator_actions else "",
    ]
  )
)
PY
    ) || return $?
  fi

  printf 'Run status: %s\n' "${report_status}"
  if [[ -n "${report_scenario}" ]]; then
    printf 'Scenario: %s\n' "${report_scenario}"
  fi
  if [[ -n "${report_run_id}" ]]; then
    printf 'Run ID: %s\n' "${report_run_id}"
  fi
  printf 'Steps: %s passed, %s failed\n' "${step_passed}" "${step_failed}"
  printf 'Assertions: %s passed, %s failed\n' "${assertion_passed}" "${assertion_failed}"
  printf 'Screenshots: expected %s, captured %s\n' "${screenshots_expected}" "${screenshots_captured}"
  if [[ -n "${next_action}" ]]; then
    printf 'Next action: %s\n' "${next_action}"
  fi
  if [[ -f "${QA_REPORT_MD_PATH}" ]]; then
    printf 'Summary report: %s\n' "${QA_REPORT_MD_PATH}"
  fi
  if [[ -n "${QA_RUN_ID}" ]]; then
    printf './aag-build-qa.sh --view %s\n' "${QA_RUN_ID}"
  fi
}
