#!/usr/bin/env zsh

log_info() {
  printf '%s\n' "$1"
}

die() {
  printf 'Error: %s\n' "$1" >&2
  exit 1
}

# Shell predicate convention:
#   return 0  -> success / true
#   return 1+ -> failure / false
#
# Predicate helpers (array_contains, is_phone_reachable, etc.) are intended
# to be used directly with if, &&, and ||.
array_contains() {
  # find a needle in the haystack
  local needle="$1"
  shift
  local item
  for item in "$@"; do
    if [[ "$item" == "$needle" ]]; then
      return 0
    fi
  done
  return 1
}
