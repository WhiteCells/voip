#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Command Error"
    echo "$0 <input.pcm> <input.wav>"
    exit 1
fi

INPUT=$1
OUTPUT=$2

ffmpeg -f s16le -ar 16000 -ac 1 -i "$INPUT" "$OUTPUT"
