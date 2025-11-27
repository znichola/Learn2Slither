#pragma once

#include <map>
#include <string>

#include "interpreter.hpp"
#include "move.hpp"

class Agent {
  typedef std::string State;
  const Move actions_key[4] = {
      Move::Up, Move::Down, Move::Left, Move::Right
  };

public:
    // state to reward (using action map)
    std::map<State, int[4]> q_table;
};

