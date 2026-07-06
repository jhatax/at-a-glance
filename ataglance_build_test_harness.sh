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
readonly BUILD_LOG_PATH="${SCRIPT_DIR}/build.log"
readonly COMPILE_DB_PATH="${SCRIPT_DIR}/compile_commands.json"
readonly COMPILE_DB_SCRIPT="${SCRIPT_DIR}/gen_compile_commands.py"

source "${QA_DIR}/lib/common.sh"
source "${QA_DIR}/lib/validate.sh"
source "${QA_DIR}/lib/pebble.sh"
source "${QA_DIR}/lib/appmessage.sh"

source "${QA_DIR}/data/all_qa_vectors.sh"

source "${QA_DIR}/stages/reset.sh"
source "${QA_DIR}/stages/compile_db.sh"
source "${QA_DIR}/stages/build.sh"
source "${QA_DIR}/stages/install.sh"

source "${QA_DIR}/tests/weather.sh"
source "${QA_DIR}/tests/battery.sh"
source "${QA_DIR}/tests/health.sh"
source "${QA_DIR}/tests/smoke.sh"

CLEAN=false
BUILD=false
INSTALL=false
WIPE=false
NUKE=false
RUN_QA=false
PHONE_INSTALL=false
PHONE_IP=""
EMULATORS=($QA_DEFAULT_EMULATORS) # Emulators can be changed based on user input
TESTS=()

print_help() {
  cat <<'EOF'
Usage: ./ataglance_build_test_harness.sh [options]

Options:
  -b,  --build            Build the project
  -bc, --build-clean      Clean first, then build
  -i,  --install          Install on selected emulators
  -p,  --phone IP         Install through Pebble mobile-app Developer Connection
  -e,  --emulators LIST   Comma-separated emulators: emery, flint, chalk, gabbro,
                          aplite, basalt, diorite
  -t,  --test LIST        Comma-separated tests: weather, battery, smoke, health
  -w,  --wipe             Wipe emulator data
  -n,  --nuclear          Kill emulators, wipe, and force a clean build
  -h,  --help             Show this help menu
EOF
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
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
        [[ -n "${2:-}" && "${2:0:1}" != "-" ]] || \
          die "-p/--phone requires a Pebble app Server IP"
        PHONE_IP="$2"
        PHONE_INSTALL=true
        shift 2
        ;;
      -e|--emulators)
        [[ -n "${2:-}" && "${2:0:1}" != "-" ]] || \
          die "-e/--emulators requires a comma-separated emulator list"
        EMULATORS=(${(s:,:)2})
        shift 2
        ;;
      -w|--wipe)
        WIPE=true
        shift
        ;;
      -t|--test)
        [[ -n "${2:-}" && "${2:0:1}" != "-" ]] || \
          die "-t/--test requires a comma-separated test list"
        RUN_QA=true
        INSTALL=true
        TESTS=(${(s:,:)2})
        shift 2
        ;;
      -n|--nuclear)
        NUKE=true
        shift
        ;;
      -h|--help)
        print_help
        exit 0
        ;;
      *)
        die "Unknown argument '$1'"
        ;;
    esac
  done
}

run_requested_tests() {
  if [[ "${RUN_QA}" != true ]]; then
    return
  fi

  local test_name
  for test_name in $TESTS; do
    case "${test_name}" in
      weather)
        qa_test_weather_run
        ;;
      battery)
        qa_test_battery_run
        ;;
      health)
        qa_test_health_run
        ;;
      smoke)
        qa_test_smoke_run
        ;;
    esac
  done
}

main() {
  parse_args "$@"
  qa_validate_config
  # Check for all possible variables in order
  qa_stage_prepare_reset_actions # NUKE, WIPE, CLEAN
  qa_stage_build_project # BUILD
  qa_stage_install_selected_emulators #INSTALL
  qa_stage_install_phone #PHONE_IP INSTALL
  run_requested_tests # QA
}

main "$@"
