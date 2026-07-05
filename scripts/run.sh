#!/bin/sh

echo ""
echo "Running Binary ..."
echo ""

# Include the common environment file for paths
. "$(dirname "$0")/env.sh" "$0"

current_path=$(pwd)

if [ ! -d "$bin_path" ]; then
    echo " ERROR! : folder \""$bin_path"\" is no exist run \"./test_build.sh\" before"
    echo ""
    exit
fi

cd "$bin_path"

# Running UNIT TESTS
for filename in $(ls -1 unit_test*); do
    echo "run executable file : $filename"
    ./"$filename"
    if [ $? -ne 0 ]; then
        echo ""
        echo "AARRGGHH!!! $filename executable failed. Exiting script."
        echo ""
        exit 1
    fi
    echo ""
done

# Running INTEGRATION TESTS
for filename in $(ls -1 integration*); do
    echo "run executable file : $filename"
    ./"$filename"
    if [ $? -ne 0 ]; then
        echo ""
        echo "AARRGGHH!!! $filename executable failed. Exiting script."
        echo ""
        exit 1
    fi
    echo ""
done

# Running EXAMPLES 
for filename in $(ls -1 example*); do
    echo "run executable file : $filename"
    ./"$filename"
    if [ $? -ne 0 ]; then
        echo ""
        echo "AARRGGHH!!! $filename executable failed. Exiting script."
        echo ""
        exit 1
    fi
    echo ""
done

cd "$current_path"

echo ""
echo "... done!"
echo ""
