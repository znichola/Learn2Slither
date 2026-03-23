#!/bin/bash

# Check if ./snake exists and is executable
if [ ! -x "./snake" ]; then
    echo "Error: ./snake not found or not executable."
    exit 1
fi

# Launch the program and send input to stdin
./snake <<EOF
TRAIN_START[go train!]TRAIN_END
EOF