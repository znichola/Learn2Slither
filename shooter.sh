#!/bin/bash

if [ ! -x "./snake" ]; then
    echo "Error: ./snake not found or not executable."
    exit 1
fi

./snake <<EOF
TRAIN_START[
{
    "EPISODES": 1000,
    "SAMPLE_PER_REPLAY": 100,
    "MAX_STEPS": 500,
    "frame_time_ms": 30,
    "board_x": 4,
    "board_y": 4,
    "alpha": 0.1,
    "gamma": 0.95,
    "epsilon": 0,
    "epsilon_decay": 0.995,
    "epsilon_min": 0,
    "statefn": "FULL",
    "state_init": "INSTANT_DEATH",
    "reward_advance": -0.21,
    "reward_green": 10,
    "reward_red": -2,
    "reward_death": -31
}
]TRAIN_END
SAVE_AGENT_START[
test_agent.txt
]SAVE_AGENT_END
EOF

# call this to continue trainig past multiple session

# RESUME_TRAIN_START[
# "continue"
# ]RESUME_TRAIN_END