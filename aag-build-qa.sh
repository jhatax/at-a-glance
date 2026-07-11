#!/usr/bin/env zsh

# At A Glance build/test harness.
# Flow and first few implementations by hand
# Refactored flow by Codex
# Outputs between my and Codex+my versions compared prior to commit
# Public entrypoint only:
#   - parse CLI
#   - validate request
#   - run stages in order
#
# Implementation details live under qa/.

set -euo pipefail

readonly SCRIPT_DIR="${0:A:h}"
readonly QA_DIR="${SCRIPT_DIR}/qa"
readonly DEFAULT_PLATFORM="emery"
readonly COMPILE_DB_PATH="${SCRIPT_DIR}/compile_commands.json"
readonly COMPILE_DB_SCRIPT="${SCRIPT_DIR}/tools/gen_compile_commands.py"

source "${QA_DIR}/lib/common.sh"
source "${QA_DIR}/data/all_qa_vectors.sh"
source "${QA_DIR}/lib/state.sh"
source "${QA_DIR}/lib/runtime.sh"
source "${QA_DIR}/lib/validate.sh"
source "${QA_DIR}/lib/pebble.sh"
source "${QA_DIR}/lib/appmessage.sh"

source "${QA_DIR}/stages/reset.sh"
source "${QA_DIR}/stages/compile_db.sh"
source "${QA_DIR}/stages/build.sh"
source "${QA_DIR}/stages/install.sh"

source "${QA_DIR}/tests/weather.sh"
source "${QA_DIR}/tests/battery.sh"
source "${QA_DIR}/tests/health.sh"

print_help() {
  cat <<'EOF'
Usage: ./aag-build-qa.sh [options]

Options:
  -b,  --build            Build the project
  -bc, --build-clean      Clean first, then build
  -i,  --install          Install on selected emulators
  -r,  --runs             List the most recent QA runs
  -v,  --view RUN         Show the existing summary report for one run
  -c,  --compare RUN...   Compare one, two, or three runs
  --validate-scenario TARGET
                          Validate a scenario name or .scenario file
  -p,  --phone IP         Install through Pebble mobile-app Developer Connection
  --scenario NAME         Run a named validation scenario
  -n,  --dry-run          Print the resolved scenario execution plan and exit
  --force                 Skip scenario execution confirmation
  --failure MODE          Inject a deliberate failure mode: build-typo
  -e,  --emulators LIST   Comma-separated emulators: emery, flint, chalk, gabbro,
                          aplite, basalt, diorite
  -w,  --wipe             Wipe emulator data
  --nuclear               Kill emulators, wipe, and force a clean build
  -h,  --help             Show this help menu
EOF
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -r|--runs)
        COMMAND_ACTION="runs"
        shift
        ;;
      -v|--view)
        COMMAND_ACTION="report"
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="-v/--view requires exactly one run selector"
          return 1
        fi
        COMMAND_ARGS=("$2")
        shift 2
        ;;
      -c|--compare)
        COMMAND_ACTION="compare"
        shift
        while [[ $# -gt 0 ]]; do
          if [[ "${1:0:1}" == "-" ]]; then
            break
          fi
          COMMAND_ARGS+=("$1")
          shift
        done
        ;;
      --validate-scenario)
        COMMAND_ACTION="validate-scenario"
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="--validate-scenario requires a scenario name or .scenario file"
          return 1
        fi
        COMMAND_ARGS=("$2")
        shift 2
        ;;
      -bc|--build-clean|--clean)
        CLEAN=true
        shift
        ;;
      -b|--build)
        BUILD=true
        shift
        ;;
      -i|--install)
        INSTALL=true
        shift
        ;;
      -p|--phone)
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="-p/--phone requires a Pebble app Server IP"
          return 1
        fi
        PHONE_IP="$2"
        PHONE_INSTALL=true
        shift 2
        ;;
      -e|--emulators)
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="-e/--emulators requires a comma-separated emulator list"
          return 1
        fi
        EMULATORS_EXPLICIT=true
        EMULATORS=(${(s:,:)2})
        shift 2
        ;;
      --scenario)
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="--scenario requires a scenario name"
          return 1
        fi
        SCENARIO_NAME="$2"
        shift 2
        ;;
      -n|--dry-run)
        DRY_RUN=true
        shift
        ;;
      --force)
        FORCE=true
        shift
        ;;
      --failure)
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="--failure requires a failure mode"
          return 1
        fi
        FAILURE_MODE="$2"
        shift 2
        ;;
      -w|--wipe)
        WIPE=true
        shift
        ;;
      --nuclear)
        NUKE=true
        shift
        ;;
      -h|--help)
        COMMAND_ACTION="help"
        shift
        ;;
      *)
        PARSE_ERROR_MESSAGE="Unknown argument '$1'"
        return 1
        ;;
    esac
  done
}

classify_request() {
  if [[ -n "${COMMAND_ACTION}" ]]; then
    COMMAND_KIND="read-only"
    return 0
  fi

  if [[ "${DRY_RUN}" == true ]]; then
    COMMAND_KIND="read-only"
    COMMAND_ACTION="dry-run"
    return 0
  fi

  if [[ -z "${SCENARIO_NAME}" ]]; then
    COMMAND_KIND="direct"
    return 0
  fi

  COMMAND_KIND="scenario-run"
  COMMAND_ACTION="scenario-run"
}

run_selected_stages() {
  qa_stage_prepare_reset_actions || return $?
  qa_stage_build_project || return $?
  qa_stage_generate_compile_database || return $?
  qa_stage_install_targets || return $?
  run_requested_tests || return $?
}

print_resolved_scenario_plan() {
  python3 "${QA_DIR}/runner.py" print-plan "${SCENARIO_NAME}" "${QA_DIR}/scenarios"
}

confirm_resolved_scenario_plan() {
  local reply

  print_resolved_scenario_plan || return $?

  if [[ "${FORCE}" == true ]]; then
    return 0
  fi

  printf 'Execute this scenario plan? [y/N] '
  read -r reply
  case "${reply:l}" in
    y|yes)
      return 0
      ;;
  esac

  printf 'Scenario execution cancelled.\n' >&2
  return 1
}

run_requested_tests() {
  qa_stage_enter "scenario-execution"

  if [[ -n "${SCENARIO_NAME}" ]]; then
    qa_run_selected_scenario
    qa_stage_pass "scenario-execution"
    return
  fi

  qa_stage_pass "scenario-execution"
}

dispatch_read_only_action() {
  case "${COMMAND_ACTION}" in
    "")
      return 1
      ;;
    help)
      print_help
      return 0
      ;;
    runs)
      python3 "${QA_DIR}/runner.py" list-runs
      return $?
      ;;
    report)
      if (( ${#COMMAND_ARGS[@]} != 1 )); then
        printf 'Error: report requires exactly one run selector\n' >&2
        return 1
      fi
      python3 "${QA_DIR}/runner.py" report "${COMMAND_ARGS[1]}"
      return $?
      ;;
    compare)
      if (( ${#COMMAND_ARGS[@]} < 1 || ${#COMMAND_ARGS[@]} > 3 )); then
        printf 'Error: compare requires one, two, or three run selectors\n' >&2
        return 1
      fi
      python3 "${QA_DIR}/runner.py" compare-runs "${COMMAND_ARGS[@]}"
      return $?
      ;;
    validate-scenario)
      if (( ${#COMMAND_ARGS[@]} != 1 )); then
        printf 'Error: validate-scenario requires exactly one scenario name or .scenario file\n' >&2
        return 1
      fi
      python3 "${QA_DIR}/runner.py" validate-scenario "${COMMAND_ARGS[1]}" "${QA_DIR}/scenarios"
      return $?
      ;;
    dry-run)
      if [[ -z "${SCENARIO_NAME}" ]]; then
        printf 'Error: -n/--dry-run requires --scenario\n' >&2
        return 1
      fi
      if [[ "${FORCE}" == true ]]; then
        printf 'Error: --force cannot be combined with -n/--dry-run\n' >&2
        return 1
      fi
      print_resolved_scenario_plan
      return $?
      ;;
  esac
}

main() {
  local parse_status=0

  parse_args "$@" || parse_status=$?

  if (( parse_status != 0 )); then
    printf 'Error: %s\n' "${PARSE_ERROR_MESSAGE}" >&2
    return "${parse_status}"
  fi

  classify_request || return $?

  if [[ "${COMMAND_KIND}" == "read-only" ]]; then
    dispatch_read_only_action
    return $?
  fi

  if [[ "${COMMAND_KIND}" == "direct" ]]; then
    if ! qa_validate_config; then
      printf 'Error: %s\n' "${QA_VALIDATION_ERROR_MESSAGE}" >&2
      return 1
    fi
    run_selected_stages
    return $?
  fi

  if [[ -n "${SCENARIO_NAME}" ]]; then
    local confirm_status=0
    confirm_resolved_scenario_plan || confirm_status=$?
    if (( confirm_status != 0 )); then
      return "${confirm_status}"
    fi
  fi

  qa_python_bootstrap || return $?
  qa_stage_enter "preflight"

  qa_assert_record "cli-legal-arguments" "passed" "arguments parsed successfully"

  if qa_validate_config; then
    qa_assert_record "cli-request-valid" "passed" "request satisfied preflight validation"
  else
    qa_assert_record "cli-request-valid" "failed" "${QA_VALIDATION_ERROR_MESSAGE}"
    printf 'Error: %s\n' "${QA_VALIDATION_ERROR_MESSAGE}" >&2
    qa_stage_fail_active
    return 1
  fi
  qa_stage_pass "preflight"
  run_selected_stages || return $?
}

typeset -g HARNESS_EXIT_STATUS=0
typeset -g CLEANUP_EXIT_STATUS=0
typeset -g FINALIZE_EXIT_STATUS=0

main "$@" || HARNESS_EXIT_STATUS=$?

if [[ "${COMMAND_KIND}" != "scenario-run" || ( ${HARNESS_EXIT_STATUS} -ne 0 && -z "${QA_RUN_ID}" ) ]]; then
  exit "${HARNESS_EXIT_STATUS}"
fi

qa_stage_cleanup_actions || CLEANUP_EXIT_STATUS=$?

if (( HARNESS_EXIT_STATUS == 0 && CLEANUP_EXIT_STATUS != 0 )); then
  HARNESS_EXIT_STATUS=${CLEANUP_EXIT_STATUS}
fi

qa_runtime_finalize "${HARNESS_EXIT_STATUS}" || FINALIZE_EXIT_STATUS=$?

if (( HARNESS_EXIT_STATUS == 0 && FINALIZE_EXIT_STATUS != 0 )); then
  HARNESS_EXIT_STATUS=${FINALIZE_EXIT_STATUS}
fi

qa_print_run_closeout

exit "${HARNESS_EXIT_STATUS}"
