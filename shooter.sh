#!/bin/bash

if [ ! -x "./snake" ]; then
    echo "Error: ./snake not found or not executable."
    exit 1
fi

./snake <<EOF
TRAIN_START[
{
    "EPISODES": 50000,
    "SAMPLE_PER_REPLAY": 5000,
    "MAX_STEPS": 800,
    "frame_time_ms": 30,
    "board_x": 10,
    "board_y": 10,
    "alpha": 0.05,
    "gamma": 0.95,
    "epsilon": 0.9,
    "epsilon_decay": 0.9999,
    "epsilon_min": 0,
    "statefn": "FULL",
    "state_init": "INSTANT_DEATH",
    "reward_advance": -0.1,
    "reward_green": 10,
    "reward_red": -2,
    "reward_death": -10
}
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
SAVE_AGENT_START[
test_agent.txt
]SAVE_AGENT_END
EOF

# call this to continue trainig past multiple session

# RESUME_TRAIN_START[
# "continue"
# ]RESUME_TRAIN_END