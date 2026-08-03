#include <cassert>
#include <algorithm>
#include <array>
#include <sstream>

#include "agent.hpp"
#include "base64.hpp"

Move Agent::chooseAction(const Vision &vision) {
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    if (dist(rng) < epsilon) {
        std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
        return static_cast<Move>(action_dist(rng));
    } else {
        const auto& q_values = getOrInsertQ(vision);
        float best_value = q_values[0];
        std::vector<int> best_indices = {0};

        for (int i = 1; i < NUM_ACTIONS; i++) {
            if (q_values[i] > best_value) {
                best_value = q_values[i];
                best_indices = {i};
            } else if (q_values[i] == best_value) {
                // build a vector of equal values, to randomly pick between them
                best_indices.push_back(i);
            }
        }

        std::uniform_int_distribution<int> dist(0, best_indices.size() - 1);
        return static_cast<Move>(best_indices[dist(rng)]);
    }
}

Move Agent::chooseActionNoUpdate(const Vision &vision) {
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    
    const auto& maybe_q_value = getQ(vision);

    if (dist(rng) < epsilon || !maybe_q_value.has_value()) {
        std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
        return static_cast<Move>(action_dist(rng));
    } else {
        float best_value = maybe_q_value.value()[0];
        std::vector<int> best_indices = {0};

        for (int i = 1; i < NUM_ACTIONS; i++) {
            if (maybe_q_value.value()[i] > best_value) {
                best_value = maybe_q_value.value()[i];
                best_indices = {i};
            } else if (maybe_q_value.value()[i] == best_value) {
                // build a vector of equal values, to randomly pick between them
                best_indices.push_back(i);
            }
        }

        std::uniform_int_distribution<int> dist(0, best_indices.size() - 1);
        return static_cast<Move>(best_indices[dist(rng)]);
    }
}


void Agent::updateQtable(const Vision &vision, Move move, float reward,
                         const Vision &next_vision) {
    auto &q_values      = getOrInsertQ(vision);
    auto &next_q_values = getOrInsertQ(next_vision);

    float max_next_q = *std::max_element(next_q_values.begin(), next_q_values.end());
    const auto action_idx = static_cast<size_t>(move);

    // Qtable update: Q(s,a) ← Q(s,a) + α * (r + γ * max_a' Q(s',a') - Q(s,a))
    q_values[action_idx] += alpha * (reward + gamma * max_next_q - q_values[action_idx]);
}

void Agent::updateQtableOnDeath(const Vision &vision, Move move, float reward) {
    auto &q_values = getOrInsertQ(vision);
    const auto action_idx = static_cast<size_t>(move);

    // Qtable update: Q(s, a) ← Q(s, a) + α * (r - Q(s, a))
    q_values[action_idx] += alpha * (reward - q_values[action_idx]);
}

void Agent::decayEpsilon() {
    epsilon = std::max(epsilon_min, epsilon * epsilon_decay);
}

/*
std::string Agent::serialiseQTable() const {
    constexpr size_t ENTRY_SIZE = sizeof(State) + sizeof(float) * NUM_ACTIONS;
    
    std::vector<uint8_t> buffer;
    buffer.reserve(q_table.size() * ENTRY_SIZE);

    for (const auto& [state, actions] : q_table) {
        const uint8_t* s = reinterpret_cast<const uint8_t*>(&state);
        buffer.insert(buffer.end(), s, s + sizeof(State));

        const uint8_t* a = reinterpret_cast<const uint8_t*>(actions.data());
        buffer.insert(buffer.end(), a, a + sizeof(float) * NUM_ACTIONS);
    }

    return base64Encode(buffer);
}

void Agent::parseQTable(const std::string& encoded) {
    constexpr size_t ENTRY_SIZE = sizeof(State) + sizeof(float) * NUM_ACTIONS;
    
    const auto buffer = base64Decode(encoded);

    if (buffer.size() % ENTRY_SIZE != 0) {
        throw std::runtime_error("Malformed Q-table: buffer size " + 
            std::to_string(buffer.size()) + " not divisible by entry size " + 
            std::to_string(ENTRY_SIZE));
    }

    q_table.clear();
    q_table.reserve(buffer.size() / ENTRY_SIZE);

    const uint8_t* ptr = buffer.data();
    const uint8_t* end = ptr + buffer.size();

    while (ptr < end) {
        State state;
        std::memcpy(&state, ptr, sizeof(State));
        ptr += sizeof(State);

        std::array<float, NUM_ACTIONS> actions;
        std::memcpy(actions.data(), ptr, sizeof(float) * NUM_ACTIONS);
        ptr += sizeof(float) * NUM_ACTIONS;

        q_table.emplace(state, actions);
    }
}
*/