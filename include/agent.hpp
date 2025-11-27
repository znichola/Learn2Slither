#pragma once

#include <map>
#include <string>

#include "interpreter.hpp"
#include "move.hpp"

class Agent {
public:
    typedef std::string State;
    const Move actions_key[4] = {
        Move::Up, Move::Down, Move::Left, Move::Right
    };

    // Values for updating the Q function
    float learning_rate = 0;
    float discount_factor = 0.9;
    float current_q = 0;
    unsigned iteration = 0;

    // state to reward (using action map)
    std::map<State, int[4]> q_table;

    Move decideOnAction(Vision vision) const;

    void updateQtable(Vision vision, int reward);
};

