#!/usr/bin/env zsh

zmodload -F zsh/stat b:stat 2> /dev/null

mtime_fmt() {
  local f="$1"

  if command stat --version > /dev/null 2>&1; then
    command stat -c '%y' -- "$f" | cut -d. -f1
  else
    command stat -f '%Sm' -t '%Y-%m-%d %H:%M:%S' -- "$f"
  fi
}

list_qa_plans() {
  printf "Listing available QA plans in %s\n" "$QAPLANS_DIR"

  local plan
  for plan in "${QAPLANS_DIR}"/*.(scenario|suite|matrix)(.N); do
    printf "%s  %s\n" "$(stat -F '%Y-%m-%d %H:%M' +mtime "$plan")" "$plan"
  done
}

list_qa_runs() {
  printf "Listing at most 10 of the most recent QA runs in %s\n" "$QARUNS_DIR"

  local run
  typeset -r -a runs=("${QARUNS_DIR}"/*(/omN[1,10]))

  if ((!${#runs[@]})); then
    printf "There are no prior qa-runs. Execute one of these plans:\n"
    list_qa_plans
    printf "Execute: <repo-root>/aag-build-qa.sh --qaplan <plan>\n"
  else
    for run in "${runs[@]}"; do
      printf "%s  %s\n" "$(mtime_fmt "$run")" "$run"
    done
  fi
}

run_command() {
  local label="$1"
  shift

  "$@"
}

install_on_phone() {
  local ip=""

  for ip in "${COMMAND_ARGS[@]}"; do
    if [[ -n "${ip}" ]]; then
      printf "Installing through Pebble Developer Connection at $ip.\n"
      pebble_install_phone "${ip}"
    fi
  done
}

# Get ready for QA plan execution
prepare_reset_actions() {
  printf "Resetting Pebble emulator state for this run.\n"
  pebble_kill || return $?
  pebble_wipe || return $?
}

# Clean up after QA plan execution
cleanup_actions() {
  printf "Cleaning up Pebble emulator state after this run.\n"
  pebble_wipe || return $?
  pebble_kill || return $?
}
