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
    float reward_death = -1.0f;

    bool _random_trigger = false;

    State::Init stateInit = State::Init::ZEROS;
    State::StateFn state;
    State::RayStateFn rayState = nullptr;

    // Random for epsilon greedy
    std::mt19937 rng{std::random_device{}()};

    std::unordered_map<std::string, std::array<float, 4>> q_table;

    Move chooseAction(const Vision & vision);
    Move chooseActionNoUpdate(const Vision & vision);
    std::string logDecision(const Vision& vision, Move chosen, const std::string &reason) const;

    void updateQtable(const Vision &vision, Move move, float reward, const Vision &next_vision);

    void updateQtableOnDeath(const Vision &vision, Move move, float reward);

    void decayEpsilon();

    std::string serialiseQTable() const;
    void parseQTable(const std::string& encoded);

    std::array<float, 4>& getOrInsertQ(const Vision &v);
    float& getOrInsertRay(const std::string &key);
    float getRayValue(const std::string &key) const;
    std::array<float, 4> getQValue(const Vision &v) const;
    std::array<std::string, 4> rayKeysByMove(const Vision &v) const;
};