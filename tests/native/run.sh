#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$test_dir/../.." && pwd)
output="$test_dir/native_tests"

case "$(uname -s)" in
  Darwin) linker_flags="-Wl,-dead_strip" ;;
  *) linker_flags="-Wl,--gc-sections" ;;
esac

cc \
  -std=c11 \
  -Wall \
  -Wextra \
  -Werror \
  -ffunction-sections \
  -I"$test_dir/include" \
  -I"$test_dir" \
  -I"$repo_dir/src" \
  -I"$repo_dir/src/modules" \
  "$test_dir/test_renderer_helper.c" \
  "$test_dir/test_runner.c" \
  "$repo_dir/src/modules/helper_tuple_parsing.c" \
  "$repo_dir/src/modules/substratum_computations.c" \
  "$linker_flags" \
  -o "$output"

trap 'rm -f "$output"' EXIT
"$output"
