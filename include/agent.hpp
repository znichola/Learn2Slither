#pragma once

#include <array>
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

    // Values for updating the Q function
    float alpha;
    float gamma;
    float epsilon;
    float epsilon_decay;
    float epsilon_min;

    // Random for epsilon greedy
    std::mt19937 rng{std::random_device{}()};

    std::unordered_map<State, std::array<float, 4>, State::Hash> q_table;

    Move chooseAction(const Vision & vision);

    void updateQtable(const Vision &vision,
                      Move move,
                      float reward,
                      const Vision &next_vision);

    void decayEpsilon();
};

