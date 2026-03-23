#include <cassert>
#include <algorithm>
#include <array>

#include "agent.hpp"


Move Agent::chooseAction(const Vision &vision) {
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    if (dist(rng) < epsilon) {
        std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
        return static_cast<Move>(action_dist(rng));
    } else {
        State state = State(vision);
        const auto& q_values = getOrInsertQ(state);
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
    
    State state = State(vision);
    const auto& maybe_q_value = getQ(state);

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
    auto state = State(vision);
    auto next_state = State(next_vision);

    auto &q_values      = getOrInsertQ(state);
    auto &next_q_values = getOrInsertQ(state);

    float max_next_q = *std::max_element(next_q_values.begin(), next_q_values.end());
    const auto action_idx = static_cast<size_t>(move);

    // Qtable update: Q(s,a) ← Q(s,a) + α * (r + γ * max_a' Q(s',a') - Q(s,a))
    q_values[action_idx] += alpha * (reward + gamma * max_next_q - q_values[action_idx]);
}

void Agent::updateQtableOnDeath(const Vision &vision, Move move, float reward) {
    auto state = State(vision);

    auto &q_values = getOrInsertQ(state);
    const auto action_idx = static_cast<size_t>(move);

    // Qtable update: Q(s, a) ← Q(s, a) + α * (r - Q(s, a))
    q_values[action_idx] += alpha * (reward - q_values[action_idx]);
}

void Agent::decayEpsilon() {
    epsilon = std::max(epsilon_min, epsilon * epsilon_decay);
}
