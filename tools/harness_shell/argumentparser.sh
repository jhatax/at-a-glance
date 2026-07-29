#!/usr/bin/env zsh

# three types of commands: 1. env-prep | 2. qa-inspection | 3. scenario-exec
parse_args() {
  DRY_RUN=false
  FORCE=false

  while [[ $# -gt 0 ]]; do
    case "$1" in
      -r | --runs)
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="runs"
        shift
        ;;
      -v | --view)
        COMMAND_TYPE="qa-inspection"
        COMMAND_ACTION="view-run"
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="-v/--view requires exactly one run selector"
          return 1
        fi
        COMMAND_ARGS=("$2")
        shift 2
        ;;
      -c | --compare)
        COMMAND_TYPE="qa-inspection"
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
      --validate | --list-steps)
        COMMAND_TYPE="qa-inspection"
        COMMAND_ACTION="validate"
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="--validate requires a QA plan name, .scenario file, or .suite file"
          return 1
        fi
        COMMAND_ARGS=("$2")
        shift 2
        ;;
      -bc | --build-clean)
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="build-clean"
        shift
        ;;
      -bv | --build-verbose)
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="build-verbose"
        shift
        ;;
      -b | --build)
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="build"
        shift
        ;;
      -p | --phone)
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="-p/--phone requires a Pebble app Server IP"
          return 1
        fi
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="install-phone"
        COMMAND_ARGS=("$2")
        shift 2
        ;;
      -e | --emulators)
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="-e/--emulators requires a comma-separated emulator list"
          return 1
        fi
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="install-emulators"
        EMULATORS=(${(s:,:)2})
        shift 2
        ;;
      --exec-plan)
        if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
          PARSE_ERROR_MESSAGE="--qaplan requires a plan name to execute"
          return 1
        fi
        COMMAND_TYPE="scenario-exec"
        COMMAND_ACTION="run-scenario"
        COMMAND_ARGS=("$2")
        shift 2
        ;;
      --plans)
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="list-qaplans"
        shift
        ;;
      -n | --dry-run)
        if [[ "${FORCE}" == true ]]; then
          PARSE_ERROR_MESSAGE="--force cannot be combined with -n/--dry-run"
          return 1
        fi
        DRY_RUN=true
        shift
        ;;
      --force)
        if [[ "$DRY_RUN" == true ]]; then
          PARSE_ERROR_MESSAGE="--force cannot be combined with -n/--dry-run"
          return 1
        fi
        FORCE=true
        shift
        ;;
      -w | --wipe)
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="wipe"
        shift
        ;;
      --nuclear)
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="nuke"
        shift
        ;;
      -h | --help)
        COMMAND_TYPE="env-prep"
        COMMAND_ACTION="help"
        shift
        ;;
      *)
        PARSE_ERROR_MESSAGE="Unknown argument '$1'"
        return 1
        ;;
    esac
  done

  if [[ "$FORCE" == true ]]; then
    COMMAND_TYPE="scenario-exec"
    COMMAND_ACTION="force-scenario"
  elif [[ "$DRY_RUN" == true ]]; then
    COMMAND_TYPE="qa-inspection"
    COMMAND_ACTION="dryrun"
  fi
}
