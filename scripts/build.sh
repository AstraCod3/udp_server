#!/bin/sh

echo ""
echo "Starting Build ..."
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
    echo "  tests          Compile only the tests (default true)"
    echo "  examples       Compile only the examples (default true)"
    echo ""
    echo " Compile examples in release mode"
    echo "Example: ./scripts/build.sh example release"
    echo ""
    echo " Compile tests in debug release mode"
    echo "Example: ./scripts/build.sh debug tests"
    exit 0
}

# Set a default build type if the user doesn't provide one
build_type="Release"
build_tests="on"
build_examples="on"

# Parse command line arguments
for arg in "$@"
do
    case "$arg" in
        -h|--help)
            show_help
            ;;
        debug)
            build_type="Debug"
            ;;
        release)
            build_type="Release"
            ;;
        tests)
            build_examples="off"
            build_tests="on"
            ;;
        examples)
            build_examples="on"
            build_tests="off"
            ;;
        "")
            # Default behavior when no argument is passed
            ;;
        *)
            echo "Error: Invalid option '$1'"
            echo "Use -h or --help to see available options."
            exit 1
            ;;
    esac
done

current_path=$(pwd)


if [ "$build_tests" = "on" ]; then
    mkdir -p $build_tests_path
    cd "$build_tests_path"

    if [ ! -d "$deps_path" ]; then
        echo "GoogleTest Framework Download and Build"
        git clone https://github.com/google/googletest.git "$deps_path"/googletest
        cmake -S "$deps_path"/googletest -B "$deps_path"/googletest/build -DCMAKE_INSTALL_PREFIX="$deps_path"/out -DINSTALL_GTEST=ON
        cmake --build "$deps_path"/googletest/build --config %build_type% --target install
    else
        echo "GoogleTest Framework already exist"
    fi

    echo ""
    echo "Building Unit Tests"
    cmake "$tests_path" -DCMAKE_BUILD_TYPE="$build_type"
    make
fi

if [ "$build_examples" = "on" ]; then
    mkdir -p $build_examples_path
    cd "$build_examples_path"
    echo ""
    echo "Building examples"
    cmake "$examples_path" -DCMAKE_BUILD_TYPE="$build_type"
    make
fi

cd $current_path

echo ""
echo "... done!"
echo ""
