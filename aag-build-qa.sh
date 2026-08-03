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

typeset -gr -a REPRESENTATIVE_EMULATORS=(gabbro emery chalk flint)
typeset -gr -a ALL_EMULATORS=(aplite basalt chalk diorite emery flint gabbro)

typeset -gr SCRIPT_DIR="${0:A:h}"
typeset -gr QAPLANS_DIR="${SCRIPT_DIR}/qa/plans"
typeset -gr QARUNS_DIR="${SCRIPT_DIR}/qa/qa-runs"
typeset -gr HELPERS="${SCRIPT_DIR}/tools"
typeset -gr DEFAULT_PLATFORM="emery"
typeset -gr DIRECT_BUILD_LOG_PATH="${SCRIPT_DIR}/build.log"

typeset -g COMMAND_TYPE=""
typeset -g COMMAND_ACTION=""
typeset -ga COMMAND_ARGS=()
typeset -g PARSE_ERROR_MESSAGE=""
typeset -ga EMULATORS=(${REPRESENTATIVE_EMULATORS[@]})
typeset -gr PYTHON_HARNESS="${HELPERS}/harness_py/ataglanceharness.py"

source "${HELPERS}/harness_shell/argumentparser.sh"
source "${HELPERS}/harness_shell/commandhandler.sh"
source "${HELPERS}/harness_shell/pebbleadapter.sh"
source "${HELPERS}/harness_shell/runtimehelper.sh"
source "${HELPERS}/harness_shell/runtimevalidator.sh"

print_help() {
  cat << 'EOF'
Usage: ./aag-build-qa.sh [options]

Options:
  -b,  --build            Build the project
  -bc, --build-clean      Clean first, then build
  -bv, --build-verbose    Build verbose, also generates compile-commands

  -e,  --emulators LIST   Install on specified csv list of emulators.
                          Supported: emery, flint, chalk, gabbro, aplite, basalt, diorite
  -p,  --phone IP         Install on phone using Pebble mobile-app Developer Connection
  -w,  --wipe             Wipe emulator data
  --nuclear               Kill emulators, wipe, and force a clean build

  -r,  --runs             List the most recent QA runs
  -v,  --view RUN         Show the existing summary report for one run
  -c,  --compare RUN...   Compare one to five runs

  --plans                 Get a list of available QA plans
  --validate QA-PLAN      Validate a QA plan by name
  --list-steps QA-PLAN    Validate a QA plan by name
  --exec-plan --exec QA-PLAN     
                          Validate and execute a named QA plan
  -n,  --dry-run          Print the resolved scenario execution plan and exit
  --force                 Skip scenario execution confirmation

  -h,  --help             Show this help menu
EOF
}

main() {
  parse_args "$@" || {
    local exit_code=$?
    printf "Error: $PARSE_ERROR_MESSAGE\n" >&2
    return $exit_code
  }

  validate_config || {
    local exit_code=$?
    printf "Error: $QA_VALIDATION_ERROR_MESSAGE\n" >&2
    return $exit_code
  }

  case "$COMMAND_TYPE" in
    env-prep)
      handle_env_prep
      ;;
    scenario-exec)
      prepare_reset_actions
      handle_qa_plan_execution
      ;;
    qa-inspection)
      handle_qa_inspections
      ;;
  esac
}

typeset -g HARNESS_EXIT_STATUS=0
typeset -g CLEANUP_EXIT_STATUS=0

main "$@" || HARNESS_EXIT_STATUS=$?

command_is_scenario_exec || exit $HARNESS_EXIT_STATUS

cleanup_actions || CLEANUP_EXIT_STATUS=$?

if ((HARNESS_EXIT_STATUS == 0 && CLEANUP_EXIT_STATUS != 0)); then
  HARNESS_EXIT_STATUS=${CLEANUP_EXIT_STATUS}
fi

exit "${HARNESS_EXIT_STATUS}"
