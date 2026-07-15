#!/bin/sh
echo " "
echo "Start Clean ..."
. "$(dirname "$0")/env.sh" "$0"
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
