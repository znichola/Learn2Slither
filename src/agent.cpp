#include <cassert>
#include <algorithm>
#include <array>
#include <sstream>
#include <iomanip>
#include <regex>

#include "agent.hpp"

static Move argmax(const std::array<float, 4>& values, std::mt19937 &rng) {
    float best_value = values[0];
    std::vector<int> best_indices = {0};

    for (int i = 1; i < Agent::NUM_ACTIONS; i++) {
        if (values[i] > best_value) {
            best_value = values[i];
            best_indices = {i};
        } else if (false && values[i] == best_value) { //  TODO check without this random
            best_indices.push_back(i);
        }
    }

    std::uniform_int_distribution<int> dist(0, best_indices.size() - 1);
    return static_cast<Move>(best_indices[dist(rng)]);
}

static bool is_fatal(const std::vector<Board::Cell>& direction) {
    return !direction.empty() &&
            (direction[0] == Board::Cell::Wall || direction[0] == Board::Cell::Snake);
};

std::array<float, 4> Agent::getQValue(const Vision &v) const {
    auto it = q_table.find(state(v));
    if (it != q_table.end()) {
        return it->second;
    }

    std::array<float, 4> init_values = {0.f, 0.f, 0.f, 0.f};

    if (stateInit == State::Init::INSTANT_DEATH) {
        if (is_fatal(v._north)) init_values[0] = reward_death;
        if (is_fatal(v._east))  init_values[1] = reward_death;
        if (is_fatal(v._south)) init_values[2] = reward_death;
        if (is_fatal(v._west))  init_values[3] = reward_death;
    }

    return init_values;
}

std::array<float, 4>& Agent::getOrInsertQ(const Vision &vision) {
    return q_table[state(vision)] = getQValue(vision);
}

float Agent::getRayValue(const std::string& key) const {
    auto it = q_table.find(key);
    if (it != q_table.end()) {
        return it->second[0];
    }

    if (stateInit == State::Init::INSTANT_DEATH && !key.empty()) {
        const char first_cell = key[0];
        if (first_cell == State::map(Board::Cell::Wall) ||
            first_cell == State::map(Board::Cell::Snake)) {
            return reward_death;
        }
    }

    return 0.f;
}

float& Agent::getOrInsertRay(const std::string& key) {
    return q_table.try_emplace(key, std::array<float,4>{getRayValue(key), 0.f, 0.f, 0.f}).first->second[0];
}

Move Agent::randomAction() {
    std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
    return static_cast<Move>(action_dist(rng));
}

Move Agent::randomSafeAction(const Vision &vision) {
    if (stateInit == State::Init::INSTANT_DEATH) {
        std::vector<Move> safe_actions;
        if (!is_fatal(vision._north)) safe_actions.push_back(Move::Up);
        if (!is_fatal(vision._east))  safe_actions.push_back(Move::Right);
        if (!is_fatal(vision._south)) safe_actions.push_back(Move::Down);
        if (!is_fatal(vision._west))  safe_actions.push_back(Move::Left);

        if (!safe_actions.empty()) {
            std::uniform_int_distribution<int> action_dist(0, static_cast<int>(safe_actions.size()) - 1);
            return safe_actions[action_dist(rng)];
        }
    }

    return randomAction();
}

Move Agent::chooseAction(const Vision &vision) {
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    _random_trigger = false;
    if (dist(rng) < epsilon) {
        _random_trigger = true;
        return randomSafeAction(vision);
    } else if (rayState) {
        auto keys = rayKeysByMove(vision);
        std::array<float, 4> values;
        for (int i = 0; i < NUM_ACTIONS; i++) values[i] = getOrInsertRay(keys[i]);
        return argmax(values, rng);
    } else {
        const auto& q_values = getOrInsertQ(vision);
        return argmax(q_values, rng);
    }
}

std::string Agent::logDecision(const Vision& vision, Move chosen, const std::string &reason) const {
    std::array<float, 4> values;
    bool wasFresh;

    if (rayState) {
        auto keys = rayKeysByMove(vision);
        wasFresh = q_table.find(keys[0]) == q_table.end();

        for (int i = 0; i < NUM_ACTIONS; i++)
            values[i] = getRayValue(keys[i]);
    } else {
        wasFresh = q_table.find(state(vision)) == q_table.end();
        values = getQValue(vision);
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Final decision: "
        << "mode=" << (rayState ? "R" : "V")
        << " eps=" << epsilon
        << " rnd=" << _random_trigger
        << " chosen=" << chosen << "[U:" << values[0] << ",R:" << values[1] << ",D:" << values[2] << ",L:" << values[3] << "]"
        << " state=" << (wasFresh ? "fresh_state" : state(vision))
        << " reson=" << reason
        ;
    return ss.str();
}

Move Agent::chooseActionNoUpdate(const Vision &vision) {
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    _random_trigger = false;
    if (dist(rng) < epsilon) {
        _random_trigger = true;
        return randomAction();
    }
    if (rayState) {
        auto keys = rayKeysByMove(vision);
        std::array<float, 4> values;
        for (int i = 0; i < NUM_ACTIONS; i++) values[i] = getRayValue(keys[i]);
        return argmax(values, rng);
    }

    const auto& keys = getQValue(vision);
    return argmax(keys, rng);
}

std::array<std::string, 4> Agent::rayKeysByMove(const Vision &v) const {
    std::array<std::string, 4> keys;
    keys[static_cast<size_t>(Move::Up)]    = rayState(v._north);
    keys[static_cast<size_t>(Move::Right)] = rayState(v._east);
    keys[static_cast<size_t>(Move::Down)]  = rayState(v._south);
    keys[static_cast<size_t>(Move::Left)]  = rayState(v._west);
    return keys;
}

void Agent::updateQtable(const Vision &vision, Move move, float reward,
                         const Vision &next_vision) {
    if (rayState) {
        auto keys = rayKeysByMove(vision);
        float &value = getOrInsertRay(keys[static_cast<size_t>(move)]);

        auto next_keys = rayKeysByMove(next_vision);
        float max_next_q = getOrInsertRay(next_keys[0]);
        for (int i = 1; i < NUM_ACTIONS; i++)
            max_next_q = std::max(max_next_q, getOrInsertRay(next_keys[i]));

        // V(ray) ← V(ray) + α * (r + γ * max_ray' V(ray') - V(ray))
        value += alpha * (reward + gamma * max_next_q - value);
        return;
    }

    auto &q_values = getOrInsertQ(vision);
    auto &next_q_values = getOrInsertQ(next_vision);

    float max_next_q = *std::max_element(next_q_values.begin(), next_q_values.end());
    const auto action_idx = static_cast<size_t>(move);

    // Qtable update: Q(s,a) ← Q(s,a) + α * (r + γ * max_a' Q(s',a') - Q(s,a))
    q_values[action_idx] += alpha * (reward + gamma * max_next_q - q_values[action_idx]);
}

void Agent::updateQtableOnDeath(const Vision &vision, Move move, float reward) {
    if (rayState) {
        auto keys = rayKeysByMove(vision);
        float &value = getOrInsertRay(keys[static_cast<size_t>(move)]);

        // V(ray) ← V(ray) + α * (r - V(ray))
        value += alpha * (reward - value);
        return;
    }

    auto &q_values = getOrInsertQ(vision);
    const auto action_idx = static_cast<size_t>(move);

    // Qtable update: Q(s, a) ← Q(s, a) + α * (r - Q(s, a))
    q_values[action_idx] += alpha * (reward - q_values[action_idx]);
}

void Agent::decayEpsilon() {
    epsilon = std::max(epsilon_min, epsilon * epsilon_decay);
}

std::string Agent::serialiseQTable() const {
    std::ostringstream res;
    res << "{";
    bool isFirst = true;
    for (const auto &[s, a] : q_table) {
        if (isFirst) {
            isFirst = false; 
        } else {
            res << ",";
        }
        res << "\n\"" << s << "\":[" << a[0] << "," << a[1] << "," << a[2] << "," << a[3] << "]";
    }
    res << "\n}";
    return res.str();
}

void Agent::parseQTable(const std::string& encoded) {
    q_table.clear();

    std::regex pairRegex(R"REGEX("([^"]+)"\s*:\s*\[([^\]]+)\])REGEX");

    for (auto it = std::sregex_iterator(encoded.begin(), encoded.end(), pairRegex);
         it != std::sregex_iterator(); ++it) {

        const std::string stateString = (*it)[1].str();
        std::stringstream ss((*it)[2].str());

        auto& weights = q_table[stateString];

        char comma;
        ss >> weights[0] >> comma
           >> weights[1] >> comma
           >> weights[2] >> comma
           >> weights[3];
    }
}