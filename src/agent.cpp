#include <cassert>
#include <algorithm>
#include <array>
#include <sstream>
#include <regex>

#include "agent.hpp"
#include "base64.hpp"

static Move argmax(const std::array<float, 4>& values, std::mt19937 &rng) {
    float best_value = values[0];
    std::vector<int> best_indices = {0};

    for (int i = 1; i < Agent::NUM_ACTIONS; i++) {
        if (values[i] > best_value) {
            best_value = values[i];
            best_indices = {i};
        } else if (values[i] == best_value) {
            // build a vector of equal values, to randomly pick between them
            best_indices.push_back(i);
        }
    }

    std::uniform_int_distribution<int> dist(0, best_indices.size() - 1);
    return static_cast<Move>(best_indices[dist(rng)]);
}

Move Agent::chooseAction(const Vision &vision) {
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    if (dist(rng) < epsilon) {
        std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
        return static_cast<Move>(action_dist(rng));
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

Move Agent::chooseActionNoUpdate(const Vision &vision) {
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    if (rayState) {
        auto keys = rayKeysByMove(vision);
        std::array<float, 4> values;
        for (int i = 0; i < NUM_ACTIONS; i++) values[i] = getRayValue(keys[i]);

        if (dist(rng) < epsilon) {
            std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
            return static_cast<Move>(action_dist(rng));
        }
        return argmax(values, rng);
    }

    const auto& maybe_q_value = getQ(vision);

    if (dist(rng) < epsilon || !maybe_q_value.has_value()) {
        std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
        return static_cast<Move>(action_dist(rng));
    }
    return argmax(maybe_q_value.value(), rng);
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

    auto &q_values      = getOrInsertQ(vision);
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
