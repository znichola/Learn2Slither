#!/bin/bash

# Check if ./snake exists and is executable
if [ ! -x "./snake" ]; then
    echo "Error: ./snake not found or not executable."
    exit 1
fi

# TRAIN_START[go train!]TRAIN_END
# Launch the program and send input to stdin
./snake <<EOF
TRAIN_START[
{"EPISODES":100,"SAMPLE_PER_REPLAY":10,"MAX_STEPS":500,"frame_time_ms":100,"board_x":10,"board_y":10,"alpha":0.1,"gamma":0.95,"epsilon":0.7,"epsilon_decay":0.995,"epsilon_min":0,"statefn":"FIRST_AND_NEXT_NON_EMPTY","reward_advance":-0.1,"reward_green":10,"reward_red":-2,"reward_death":-10}
]TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
RESUME_TRAIN_START[
"continue"
]RESUME_TRAIN_END
EOF