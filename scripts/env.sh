#!/bin/sh

export script_path=$(cd "$(dirname "$0")" && pwd)
export root_path="$script_path/.."
export build_path="$root_path/build"
export bin_path="$root_path/bin"
export deps_path="$root_path/deps"
export log_path="$root_path/log"
export test_path="$root_path/test"
