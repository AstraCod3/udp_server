#!/bin/sh

export script_path=$(cd "$(dirname "$0")" && pwd)
export root_path="$script_path/.."
export build_examples_path="$root_path/build/examples"
export build_tests_path="$root_path/build/tests"
export bin_path="$root_path/bin"
export deps_path="$root_path/deps"
export log_path="$root_path/log"
export examples_path="$root_path/examples"
export tests_path="$root_path/tests"
