#include <cassert>
#include <algorithm>
#include <array>

#include "agent.hpp"


Move Agent::chooseAction(const Vision &vision) {
    auto state = State(vision);

    if (q_table.find(state) == q_table.end()) {
        q_table[state] = {0.f, 0.f, 0.f, 0.f};
    }

    std::uniform_real_distribution<float> dist(0.f, 1.f);

    if (dist(rng) < epsilon) {
        // Exploring a random action
        std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
        return actions_key[action_dist(rng)];
    } else {
        // Exploit the best possible action
        const auto& q_values = q_table[state];
        int best_index = 0;
        for (int i = 0; i < NUM_ACTIONS; i++) {
            if (q_values[i] > q_values[best_index]) {
                best_index = i;
            }
        }
        return actions_key[best_index];
    }
}


void Agent::updateQtable(const Vision &vision, Move move, float reward,
                         const Vision &next_vision) {
    auto state = State(vision);
    auto next_state = State(next_vision);

    if (q_table.find(state) == q_table.end()) {
        q_table[state] = {0.f, 0.f, 0.f, 0.f};
    }
    if (q_table.find(next_state) == q_table.end()) {
        q_table[next_state] = {0.f, 0.f, 0.f, 0.f};
    }

    auto &q_values = q_table[state];
    auto &next_q_values = q_table[next_state];

    auto placeholder =
        *std::max_element(next_q_values.begin(), next_q_values.end());
    float max_next_q = placeholder;

    auto it = std::find(std::begin(actions_key), std::end(actions_key), move);
    assert(it != std::end(actions_key));
    auto action_idx = std::distance(std::begin(actions_key), it);

    // Ql update: Q(s,a) ← Q(s,a) + α * (r + γ * max_a' Q(s',a') - Q(s,a))
    q_values[action_idx] += 
        alpha * (reward + gamma * max_next_q - q_values[action_idx]);
}

