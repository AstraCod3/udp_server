#!/bin/sh

echo ""
echo "Start Script Build Test ..."
echo ""

# Include the common environment file for paths
. "$(dirname "$0")/env.sh" "$0"

# Function to display the help menu
show_help() {
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Available options:"
    echo "  -h, --help     Print this menu and exit"
    echo "  debug          Compile the project in Debug mode (with symbols)"
    echo "  release        Compile the project in Release mode (optimized)"
    echo ""
    echo "Example: ./scripts/build.sh release"
    exit 0
}

# Set a default build type if the user doesn't provide one
build_type="Release"

# Parse command line arguments
case "$1" in
    -h|--help)
        show_help
        ;;
    debug)
        build_type="Debug"
        ;;
    release)
        build_type="Release"
        ;;
    "")
        # Default behavior when no argument is passed
        build_type="Release"
        ;;
    *)
        echo "Error: Invalid option '$1'"
        echo "Use -h or --help to see available options."
        exit 1
        ;;
esac

current_path=$(pwd)

if [ ! -d "$deps_path" ]; then
    echo "GoogleTest Download and Build"
    git clone https://github.com/google/googletest.git "$deps_path"/googletest
    cmake -S "$deps_path"/googletest -B "$deps_path"/googletest/build -DCMAKE_INSTALL_PREFIX="$deps_path"/out -DINSTALL_GTEST=ON
    cmake --build "$deps_path"/googletest/build --config %build_type% --target install
else
    echo "GoogleTest already exist"
fi

echo ""
mkdir -v $build_path
cd "$build_path"

echo ""
cmake "$test_path" -DCMAKE_BUILD_TYPE="$build_type"
echo ""
make

echo ""
mkdir -v $bin_path
cp -vf "test_udp_server" "$bin_path"

cd $current_path

echo ""
echo "... done!"
echo ""
