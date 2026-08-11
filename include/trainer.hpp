#pragma once

#include "environment.hpp"
#include "interpreter.hpp"
#include "agent.hpp"

#include <vector>
#include <limits>
#include <cmath>

class Trainer {
public:
    struct Config {
        unsigned EPISODES = 9000;
        unsigned SAMPLE_PER_REPLAY = EPISODES / 10;
        unsigned frame_time_ms = 100; 
        unsigned MAX_STEPS = 2000;
        unsigned board_x = 10;
        unsigned board_y = 10;

        float alpha = 0.4f;
        float gamma = 0.9f;
        float epsilon = 0.1f;
        float epsilon_decay = 0.995f;
        float epsilon_min = 0.01f;
        State::Type stateFn = State::Type::FIRST_NON_EMPTY;
        State::Init stateInit = State::Init::ZEROS;

        float reward_advance = 0.0f;
        float reward_green = 10000.0f;
        float reward_red = 1.0f;
        float reward_death = -10.0f;
    };

    struct EpisodeResult {
        enum class EndReason { Death, MaxSteps, NoFoodTimeout };
        inline static std::string reasonToString(EndReason reason) {
            switch (reason) {
            case Trainer::EpisodeResult::EndReason::Death: return "death";
            case Trainer::EpisodeResult::EndReason::MaxSteps: return "max_steps";
            case Trainer::EpisodeResult::EndReason::NoFoodTimeout: return "no_food_timeout";
            }
            return "unknown";
        }
        unsigned steps;
        unsigned length;
        Move lastMove;
        EndReason reason;
    };

    struct EpochStats {
        unsigned count = 0;

        unsigned maxLength = 0;
        unsigned minLength = std::numeric_limits<unsigned>::max();
        double   meanLength = 0.0;
        double   m2Length = 0.0;

        unsigned maxSteps = 0;
        unsigned minSteps = std::numeric_limits<unsigned>::max();
        double   meanSteps = 0.0;
        double   m2Steps = 0.0;

        void add(unsigned length, unsigned steps) {
            maxLength = std::max(maxLength, length);
            minLength = std::min(minLength, length);
            maxSteps  = std::max(maxSteps, steps);
            minSteps  = std::min(minSteps, steps);

            ++count;
            double dLen = length - meanLength;
            meanLength += dLen / count;
            m2Length   += dLen * (length - meanLength);

            double dSteps = steps - meanSteps;
            meanSteps += dSteps / count;
            m2Steps   += dSteps * (steps - meanSteps);
        }

        double stddevLength() const { return count > 1 ? std::sqrt(m2Length / (count - 1)) : 0.0; }
        double stddevSteps()  const { return count > 1 ? std::sqrt(m2Steps  / (count - 1)) : 0.0; }
    };

    Config config;

    Agent agent;
    Pipe pipe;

    Trainer() = delete;
    Trainer(const Config &config);

    void AIplay();
    void train();
    EpisodeResult trainEpisode(bool log);

    float reward(MoveRes moveRes);

    unsigned _current_ep = 0;

    void updateConfig(const Config &config);

    std::vector<Board> currentBoards;
};
