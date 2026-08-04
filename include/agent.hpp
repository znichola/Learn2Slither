#pragma once

#include <array>
#include <unordered_map>
#include <string>
#include <random>
#include <optional>

#include "interpreter.hpp"
#include "move.hpp"

class Agent {
public:
    static constexpr int NUM_ACTIONS = 4;

    // Values for updating the Q function
    float alpha;
    float gamma;
    float epsilon;
    float epsilon_decay;
    float epsilon_min;

    State::StateFn state;

    // Random for epsilon greedy
    std::mt19937 rng{std::random_device{}()};

    std::unordered_map<std::string, std::array<float, 4>> q_table;

    Move chooseAction(const Vision & vision);
    Move chooseActionNoUpdate(const Vision & vision);

    void updateQtable(const Vision &vision,
                        Move move,
                        float reward,
                        const Vision &next_vision);

    void updateQtableOnDeath(const Vision &vision,
                        Move move, 
                        float reward);

    void decayEpsilon();

    std::string serialiseQTable() const;
    void parseQTable(const std::string& encoded);
    // NOTE also remember to pass on the epsilon value, as this is modified during an agent liftime
    // NOTE also must pass the state function

private:
    std::array<float, 4>& getOrInsertQ(const Vision &v) {
        return q_table.try_emplace(state(v), std::array<float,4>{}).first->second;
    }

    std::optional<std::array<float, 4>> getQ(const Vision &v) const {
        auto it = q_table.find(state(v));
        if (it == q_table.end()) return std::nullopt;
        return it->second;
    }
};

