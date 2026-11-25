#!/bin/sh
set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$test_dir/../.." && pwd)
export PYTHONDONTWRITEBYTECODE=1
export LC_ALL=C
suite_filter=all
case_filter=
matrix=commit
platform_filter=
health_filter=
list=0
keep=0
sanitize=${NATIVE_SANITIZE:-0}

usage() {
  cat << 'HELP'
Usage: tests/native/run.sh [options]
  --matrix NAME      commit (default): emery/chalk/flint health, aplite no-health
                     full: all seven platforms, with and without health
  --suite NAME       Run one suite (default: all)
  --case NAME        Run one exact case; requires --suite
  --platform NAME    Override the matrix platforms with one platform, or all seven
  --health VARIANT   Override the matrix health selection: no-health, health, both
  --list             List selected cases without compiling
  --sanitize         Enable AddressSanitizer and UndefinedBehaviorSanitizer
  --keep-build       Keep binaries and logs under build/native-tests
  --bounds           Print calculated layout rectangles and circle observations
  --help             Show this help
Platforms: aplite, basalt, chalk, diorite, emery, flint, gabbro
HELP
}
while [ "$#" -gt 0 ]; do
  case "$1" in
    --matrix | --suite | --case | --platform | --health)
      [ "$#" -ge 2 ] && [ -n "$2" ] || {
        printf 'Option %s requires a nonempty value\n' "$1" >&2
        exit 2
      }
      case "$1" in
        --matrix) matrix=$2 ;;
        --suite) suite_filter=$2 ;;
        --case) case_filter=$2 ;;
        --platform) platform_filter=$2 ;;
        --health) health_filter=$2 ;;
      esac
      shift 2
      ;;
    --list)
      list=1
      shift
      ;;
    --sanitize)
      sanitize=1
      shift
      ;;
    --keep-build)
      keep=1
      shift
      ;;
    --bounds)
      export NATIVE_LAYOUT_BOUNDS=1
      shift
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown option: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
done
[ -z "$case_filter" ] || [ "$suite_filter" != all ] || {
  printf '%s\n' '--case requires --suite' >&2
  exit 2
}
case "$matrix" in
  commit) platforms='emery chalk aplite flint' ;;
  full) platforms='aplite basalt chalk diorite emery flint gabbro' ;;
  *)
    printf 'Unknown matrix: %s\n' "$matrix" >&2
    exit 2
    ;;
esac
case "$platform_filter" in
  '') ;;
  all) platforms='aplite basalt chalk diorite emery flint gabbro' ;;
  aplite | basalt | chalk | diorite | emery | flint | gabbro) platforms=$platform_filter ;;
  *)
    printf 'Unknown platform: %s\n' "$platform_filter" >&2
    exit 2
    ;;
esac
case "$health_filter" in
  '' | both | no-health | health) ;;
  *)
    printf 'Unknown health variant: %s\n' "$health_filter" >&2
    exit 2
    ;;
esac

# Select once. Discovery and registration use the same upstream naming rules.
selected_suites=
selected_cases=
for source in "$test_dir"/suites/*.c; do
  suite=$(basename "$source" .c)
  [ "$suite_filter" = all ] || [ "$suite_filter" = "$suite" ] || continue
  # Clar treats underscores as namespace separators and selects the first match.
  for selected_suite in $selected_suites; do
    shared_namespace=0
    case "$suite" in "$selected_suite"_*) shared_namespace=1 ;; esac
    case "$selected_suite" in "$suite"_*) shared_namespace=1 ;; esac
    [ "$shared_namespace" = 0 ] || {
      printf 'Conflicting suite names: %s and %s; rename one or select a single suite\n' "$selected_suite" "$suite" >&2
      exit 2
    }
  done
  if [ -n "$case_filter" ]; then
    names=$(python3 "$test_dir/discover.py" "$source" "$case_filter")
  else
    names=$(python3 "$test_dir/discover.py" "$source")
  fi
  selected_suites="$selected_suites $suite"
  selected_cases="$selected_cases $names"
  if [ "$list" = 1 ]; then printf '%s\n' "$names"; fi
done
[ -n "$selected_suites" ] || {
  printf 'No matching suite: %s\n' "$suite_filter" >&2
  exit 2
}
[ "$list" = 0 ] || exit 0

if [ "$keep" = 1 ]; then
  build_root="$repo_dir/build/native-tests"
  mkdir -p "$build_root"
else
  build_root=$(mktemp -d "${TMPDIR:-/tmp}/aag-native.XXXXXX")
  trap 'rm -rf "$build_root"' EXIT HUP INT TERM
fi

# Suite-to-source table. Add an entry here when adding a suite.
for suite in $selected_suites; do
  case "$suite" in
    comms)
      printf '%s\n' src/c/ataglance_comms_handler.c src/c/ataglance_message_parser.c \
        src/modules/watchface_runtime_boundary.c src/modules/settings.c
      ;;
    helpers)
      printf '%s\n' src/c/ataglance_message_parser.c src/modules/substratum_computations.c
      ;;
    layout)
      printf '%s\n' src/modules/layout_architect.c src/modules/substratum_computations.c
      ;;
    parser)
      printf '%s\n' src/c/ataglance_message_parser.c
      ;;
    runtime)
      printf '%s\n' src/modules/watchface_runtime_boundary.c src/modules/settings.c
      ;;
    settings)
      printf '%s\n' src/modules/settings.c
      ;;
    *)
      printf 'Native C: no production source mapping for %s; skipping source entry\n' "$suite" >&2
      continue
      ;;
  esac
done > "$build_root/dependencies.unsorted"
sort -u "$build_root/dependencies.unsorted" > "$build_root/dependencies"

# Registration does not depend on platform or health settings.
registration="$build_root/registration"
generator_source="$test_dir/suites"
[ "$suite_filter" = all ] || generator_source="$test_dir/suites/$suite_filter.c"
mkdir -p "$registration"
python3 "$test_dir/vendor/clar/generate.py" -f -o "$registration" "$generator_source" > "$registration/generate.log"
case "$(uname -s)" in
  Darwin) linker_flags='-Wl,-dead_strip' ;;
  *) linker_flags='-Wl,--gc-sections' ;;
esac
: > "$build_root/commands.fragments"
run_list="$build_root/run-list.tsv"
: > "$run_list"
failures=0
builds=0
executions=0

for platform in $platforms; do
  case "$platform" in
    aplite | diorite | flint)
      shape=RECT
      color=BW
      width=144
      height=168
      ;;
    basalt)
      shape=RECT
      color=COLOR
      width=144
      height=168
      ;;
    chalk)
      shape=ROUND
      color=COLOR
      width=180
      height=180
      ;;
    emery)
      shape=RECT
      color=COLOR
      width=200
      height=228
      ;;
    gabbro)
      shape=ROUND
      color=COLOR
      width=260
      height=260
      ;;
  esac
  platform_macro=$(printf '%s' "$platform" | tr '[:lower:]' '[:upper:]')
  if [ -n "$health_filter" ]; then
    variants=$health_filter
  elif [ "$matrix" = full ]; then
    variants=both
  elif [ "$platform" = aplite ]; then
    variants=no-health
  else
    variants=health
  fi
  [ "$variants" != both ] || variants='no-health health'
  for variant in $variants; do
    build="$build_root/$platform-$variant"
    mkdir -p "$build"
    # A failed rebuild must never execute a retained binary from an earlier run.
    rm -f "$build/native_tests"
    for identifier in $selected_cases; do
      suite=${identifier%%::*}
      test_case=${identifier#*::}
      printf '%s\t%s\t%s\t%s\n' "$platform" "$variant" "$suite" "$test_case" >> "$run_list"
    done
    set -- "-DPBL_PLATFORM_$platform_macro" "-DPBL_$shape" "-DPBL_$color" \
      "-DPBL_DISPLAY_WIDTH=$width" "-DPBL_DISPLAY_HEIGHT=$height" "-DTEST_PLATFORM=\"$platform\""
    [ "$variant" != health ] || set -- "$@" -DPBL_HEALTH
    [ "$sanitize" != 1 ] || set -- "$@" -DTEST_SANITIZE -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
    for suite in $selected_suites; do
      set -- "$@" "$test_dir/suites/$suite.c"
    done
    set -- "$@" "$test_dir/test_runner.c" "$test_dir/test_mocks.c" "$test_dir/vendor/clar/clar.c"
    while IFS= read -r dependency; do
      set -- "$@" "$repo_dir/$dependency"
    done < "$build_root/dependencies"
    printf '\nNative C: building %s / %s (%sx%s)\n' "$platform" "$variant" "$width" "$height"
    builds=$((builds + 1))
    if ! clang -std=c11 -g -O0 -fshort-enums -Wall -Wextra -Werror -Wno-unused-parameter \
      -ffunction-sections -I"$test_dir/include" -I"$test_dir" -I"$test_dir/vendor/clar" \
      -I"$registration" -I"$repo_dir/src" -iquote "$repo_dir/src/modules" \
      -MJ "$build/commands.fragment" "$@" "$linker_flags" -lm -o "$build/native_tests" > "$build/build.log" 2>&1; then
      cat "$build/build.log" >&2
      failures=$((failures + 1))
      continue
    fi
    cat "$build/commands.fragment" >> "$build_root/commands.fragments"

  done
done

# One ordered list handles every scope. Only the selected case runs in each process.
current_group=
tab=$(printf '\t')
while IFS="$tab" read -r platform variant suite test_case; do
  group="$platform/$variant/$suite"
  if [ "$group" != "$current_group" ]; then
    printf '\nNative C: running %s / %s / %s\n' "$suite" "$platform" "$variant"
    clar_suite=$(printf '%s' "$suite" | sed 's/_/::/g')
    current_group=$group
  fi
  build="$build_root/$platform-$variant"
  case_log="$build/$suite.$test_case.log"
  if [ ! -x "$build/native_tests" ]; then
    printf '%s::%s: not run (build failed)\n' "$suite" "$test_case" > "$case_log"
    cat "$case_log"
    continue
  fi
  executions=$((executions + 1))
  case_status=0
  { "$build/native_tests" -v -v "-s$clar_suite::$test_case\$"; } > "$case_log" 2>&1 || case_status=$?
  # The operator header above identifies the selected suite; keep Clar unchanged.
  sed '/^Loaded [0-9][0-9]* suites:$/d; /^Started (test status codes: /d; /^$/d' "$case_log"
  if [ "$case_status" -ne 0 ]; then
    failures=$((failures + 1))
    printf '\n%s::%s: fail (process status %s)\n' "$suite" "$test_case" "$case_status" >&2
    printf 'FAILED: npm run test:native -- --suite %s --case %s --platform %s --health %s' "$suite" "$test_case" "$platform" "$variant" >&2
    [ "$sanitize" != 1 ] || printf ' --sanitize' >&2
    printf '\n' >&2
  fi
done < "$run_list"
{
  printf '[\n'
  sed '$s/,$//' "$build_root/commands.fragments"
  printf ']\n'
} > "$test_dir/compile_commands.json"
printf '\nNative C: %s builds, %s case executions, %s failures\n' "$builds" "$executions" "$failures"
[ "$keep" != 1 ] || printf 'Debug binaries and logs: %s\n' "$build_root"
[ "$failures" = 0 ]
