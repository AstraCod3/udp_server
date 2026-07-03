#!/bin/sh
echo " "
echo "Start Clean ..."

. "$(dirname "$0")/env.sh" "$0"

export build_path="$script_path/../build"
export bin_path="$script_path/../bin"
export deps_path="$script_path/../deps"
export log_path="$script_path/../log"

if [ -d "$build_path" ]; then
    echo "remove directory: $build_path"
    rm -rf "$build_path"
fi

if [ -d "$bin_path" ]; then
    echo "remove directory: $bin_path"
    rm -rf "$bin_path"
fi

if [ -d "$deps_path" ]; then
    echo "remove directory: $deps_path"
    rm -rf "$deps_path"
fi

if [ -d "$log_path" ]; then
    echo "remove directory: $log_path"
    rm -rf "$log_path"
fi

echo " "
echo "... done!"
echo " "
