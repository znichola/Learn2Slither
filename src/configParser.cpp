#include <regex>
#include <string>

#include "configParser.hpp"
#include "logger.hpp"

Trainer::Config parseConfig(const std::string &configStr) {
    Trainer::Config config;

    // Regex to match: "key" : number
    // Captures:
    // 1 -> key
    // 2 -> numeric value (int or float, including negative)
    std::regex pairRegex(R"REGEX("([^"]+)"\s*:\s*(-?\d+(\.\d+)?))REGEX");

    auto begin = std::sregex_iterator(configStr.begin(), configStr.end(), pairRegex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;

        std::string key = match[1].str();
        std::string valueStr = match[2].str();

        // Convert to float first (works for both int and float)
        float value = std::stof(valueStr);

        // Map key to struct fields
        if (key == "EPISODES") config.EPISODES = static_cast<unsigned>(value);
        else if (key == "BATCH_SIZE") config.BATCH_SIZE = static_cast<unsigned>(value);
        else if (key == "MAX_STEPS") config.MAX_STEPS = static_cast<unsigned>(value);
        else if (key == "frame_time_ms") config.frame_time_ms = static_cast<unsigned>(value);
        else if (key == "board_x") config.board_x = static_cast<unsigned>(value);
        else if (key == "board_y") config.board_y = static_cast<unsigned>(value);

        else if (key == "alpha") config.alpha = value;
        else if (key == "gamma") config.gamma = value;
        else if (key == "epsilon") config.epsilon = value;
        else if (key == "epsilon_decay") config.epsilon_decay = value;
        else if (key == "epsilon_min") config.epsilon_min = value;

        else if (key == "reward_advance") config.reward_advance = value;
        else if (key == "reward_green") config.reward_green = value;
        else if (key == "reward_red") config.reward_red = value;
        else if (key == "reward_death") config.reward_death = value;

        else {
            Logger::error() << "Warning: Unrecognized config key: " << key << std::endl;
        }
    }

    return config;
}
