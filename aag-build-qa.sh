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
typeset -gr QA_DIR="${SCRIPT_DIR}/qa"
typeset -gr DEFAULT_PLATFORM="emery"
typeset -gr DIRECT_BUILD_LOG_PATH="${SCRIPT_DIR}/build.log"
typeset -gr COMPILE_DB_PATH="${SCRIPT_DIR}/compile_commands.json"
typeset -gr COMPILE_DB_SCRIPT="${SCRIPT_DIR}/tools/gen_compile_commands.py"

typeset -g COMMAND_TYPE=""
typeset -g COMMAND_ACTION=""
typeset -ga COMMAND_ARGS=()
typeset -g PARSE_ERROR_MESSAGE=""
typeset -ga EMULATORS=(${REPRESENTATIVE_EMULATORS[@]})


source "${QA_DIR}/lib/argumentparser.sh"
source "${QA_DIR}/lib/commandhandler.sh"
source "${QA_DIR}/lib/pebbleadapter.sh"
source "${QA_DIR}/lib/runtimehelper.sh"
source "${QA_DIR}/lib/runtimevalidator.sh"

print_help() {
  cat <<'EOF'
Usage: ./aag-build-qa.sh [options]

Options:
  -b,  --build            Build the project
  -bc, --build-clean      Clean first, then build
  -bv, --build-verbose    Build verbose, also generates compile-commands; use with -bc for most effect
  -i,  --install          Install on selected emulators
  -r,  --runs             List the most recent QA runs
  -v,  --view RUN         Show the existing summary report for one run
  -c,  --compare RUN...   Compare one, two, or three runs
  --validate QA-PLAN      Validate a QA plan by name
  -p,  --phone IP         Install through Pebble mobile-app Developer Connection
  --qaplan QA-PLAN        Validate and run a named QA plan
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

if (( HARNESS_EXIT_STATUS == 0 && CLEANUP_EXIT_STATUS != 0 )); then
  HARNESS_EXIT_STATUS=${CLEANUP_EXIT_STATUS}
fi

exit "${HARNESS_EXIT_STATUS}"
