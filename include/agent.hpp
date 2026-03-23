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
    Move chooseActionNoUpdate(const Vision & vision);

    void updateQtable(const Vision &vision,
                        Move move,
                        float reward,
                        const Vision &next_vision);

    void updateQtableOnDeath(const Vision &vision,
                        Move move, 
                        float reward);

    void decayEpsilon();

private:
    std::array<float, 4>& getOrInsertQ(const State& state) {
        return q_table.try_emplace(state, std::array<float,4>{}).first->second;
    }

    std::optional<std::array<float, 4>> getQ(const State& state) const {
        auto it = q_table.find(state);
        if (it == q_table.end()) return std::nullopt;
        return it->second;
    }
};

