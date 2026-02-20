#pragma once

#include "environment.hpp"
#include "interpreter.hpp"
#include "agent.hpp"


class Trainer {
public:
    struct Config {
        unsigned EPISODES = 9000;
        unsigned BATCH_SIZE = EPISODES / 10;
        unsigned frame_time_ms = 100; 
        unsigned MAX_STEPS = 2000;
        unsigned board_x = 10;
        unsigned board_y = 10;


        // Agent config
        float alpha = 0.4f;             // learning rate : 0 < a <= 1
        float gamma = 0.9f;             // discount factor : 0 < g <= 1
        float epsilon = 0.1f;           // exploration random probability
        float epsilon_decay = 0.995f;   // decay rate for epsilon
        float epsilon_min = 0.01f;

        // reward config
        float reward_advance = 0.0f;
        float reward_green = 10000.0f;
        float reward_red = 1.0f;
        float reward_death = -10.0f;
    };
    
    Config config;

    Agent agent;
    Pipe pipe;

    Trainer() = delete;
    Trainer(const Config &config);

    void AIplay();
    void train();
    void trainEpisode(bool log);
    
    float reward(MoveRes moveRes);

    unsigned _current_ep = 0;
};