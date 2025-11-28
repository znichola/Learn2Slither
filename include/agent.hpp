#pragma once

#include <unordered_map>
#include <string>
#include <random>

#include "interpreter.hpp"
#include "move.hpp"

class Agent {
public:
    static constexpr int NUM_ACTIONS = 4;
    static constexpr Move actions_key[NUM_ACTIONS] = {
        Move::Up, Move::Down, Move::Left, Move::Right
    };

    std::unordered_map<State, std::array<float, 4>, State::Hash> q_table;

    // Values for updating the Q function
    float alpha = 0.4f;       // learning rate : 0 < a <= 1
    float gamma = 0.9f;       // discount factor : 0 < g <= 1
    float epsilon = 0.1f;     // exploration random probability
    unsigned max_iter = 1000; // to combat infinate loops
    unsigned _iteration = 0;

    // Random for epsilon greedy
    std::mt19937 rng{std::random_device{}()};

    Move decideOnAction(const Vision & vision);

    void updateQtable(const Vision &vision, Move move, float reward,
                      const State &next_state);
};

