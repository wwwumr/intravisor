#!/bin/bash

if [ $# -ne 4 ]; then
    echo "Usage: $0 platform timeout output input_keys"
    exit 1
fi

platform=$1
timeout=$2
output=$3
input_keys=$4

setsid /home/cheri/cheribuild/cheribuild.py run-$platform-hybrid --enable-hybrid-targets --cheribsd/git-revision=release/22.05p1 --skip-update > $output &

# Store the process ID (PID) of the last background process
pid=$!

# Wait for 20 seconds
sleep $timeout

# Terminate the program using the stored PID
kill -9 -$pid

killall -9 qemu-system-$platform

iconv -f ISO-8859-1 -t UTF-8 $output > /tmp/tmp.log

python3 ./qemu-parse.py  /tmp/tmp.log $input_keys intr

