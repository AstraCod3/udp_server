#!/bin/bash

# Include the common environment file for paths
. "$(dirname "$0")/env.sh" "$0"

echo ""
echo "Start Running with Valgrind"
echo ""

log_postfix="$(date +"%Y%m%d_%H%M%S").log"

check_memory_leask() {
    bin_file="$1"
    log_file="$log_path/$bin_file".memory_leak."$log_postfix"
    # cms_valg_memleak="valgrind --error-limit=no --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=$log_file ./$bin_file"
    cms_valg_memleak="valgrind --error-limit=no --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./$bin_file > $log_file 2>&1"
    echo "Valgrind Test Memory Leak"
    echo " > $cms_valg_memleak"
    eval "$cms_valg_memleak"
    #eval "$cms_valg_memleak" > $log_file 2>&1
    echo ""
}

check_race_condition_drd() {
    bin_file="$1"
    log_file="$log_path/$bin_file.drd.$log_postfix"
    # cmd_valg_drd="valgrind --error-limit=no --tool=drd -s --log-file="$log_file" ./"$bin_file""
    cmd_valg_drd="valgrind --error-limit=no --tool=drd -s ./$bin_file > $log_file 2>&1"
    echo "Valgrind Test drd run"
    echo " > $cmd_valg_drd"
    eval $cmd_valg_drd
    echo ""
}

check_race_condition_helgrind() { 
    bin_file="$1"
    log_file="$log_path/$bin_file".helgrind."$log_postfix"
    # cmd_valg_helgrind="valgrind --error-limit=no --tool=helgrind --history-level=approx --log-file="$log_file" -s ./"$bin_file""
    cmd_valg_helgrind="valgrind --error-limit=no --tool=helgrind --history-level=approx -s ./$bin_file > $log_file 2>&1"
    echo "Valgrind Test helgrind"
    echo " > $cmd_valg_helgrind"
    eval "$cmd_valg_helgrind"
    echo ""
}

if [ ! -d "$bin_path" ]; then
    echo " ERROR! : folder "$bin_path" is no exist run \"./test_build.sh\" before"
    echo ""
    exit
fi


if [ ! -d "$log_path" ]; then
    mkdir -v "$log_path"
fi
echo ""


cd "$bin_path"
for filename in $(ls -1); do
    echo ""
    echo "running : $filename"
    echo ""
    check_memory_leask "$filename"
    check_race_condition_drd "$filename"
    check_race_condition_helgrind "$filename"
    echo ""
done

cd ..
echo " "
echo "... done!"
echo " "
