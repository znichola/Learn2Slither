#include <regex>
#include <string>

#include "configParser.hpp"
#include "logger.hpp"

Trainer::Config parseConfig(const std::string &configStr) {
    Trainer::Config config;

    // Regex to match: "key" : number or "key" : "value"
    // Captures:
    // 1 -> key
    // 2 -> number as string (int or float, including negative)
    // 3 -> string value with no quotes
    std::regex pairRegex(R"REGEX("([^"]+)"\s*:\s*(?:(-?\d+(?:\.\d+)?)|"([^"]+)"))REGEX");

    auto begin = std::sregex_iterator(configStr.begin(), configStr.end(), pairRegex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;

        std::string key = match[1].str();
        std::string valueStr = match[2].str();
        float value = 0.0;

        if (valueStr == "") {
            valueStr = match[3].str();
        } else {
            value = std::stof(valueStr);
        }

        // Map key to struct fields
        if (key == "EPISODES") config.EPISODES = static_cast<unsigned>(value);
        else if (key == "SAMPLE_PER_REPLAY") config.SAMPLE_PER_REPLAY = static_cast<unsigned>(value);
        else if (key == "MAX_STEPS") config.MAX_STEPS = static_cast<unsigned>(value);
        else if (key == "frame_time_ms") config.frame_time_ms = static_cast<unsigned>(value);
        else if (key == "board_x") config.board_x = static_cast<unsigned>(value);
        else if (key == "board_y") config.board_y = static_cast<unsigned>(value);
        else if (key == "ai_runs") config.ai_runs = static_cast<unsigned>(value);

        else if (key == "alpha") config.alpha = value;
        else if (key == "gamma") config.gamma = value;
        else if (key == "epsilon") config.epsilon = value;
        else if (key == "epsilon_decay") config.epsilon_decay = value;
        else if (key == "epsilon_min") config.epsilon_min = value;
        else if (key == "statefn") config.stateFn = State::parse(valueStr);
        else if (key == "state_init") config.stateInit = State::parseInit(valueStr);

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


std::string serialiseConfig(const Trainer::Config &config) {
    std::ostringstream oss;
    oss << "{"
        << "\"EPISODES\":" << config.EPISODES << ","
        << "\"SAMPLE_PER_REPLAY\":" << config.SAMPLE_PER_REPLAY << ","
        << "\"MAX_STEPS\":" << config.MAX_STEPS << ","
        << "\"frame_time_ms\":" << config.frame_time_ms << ","
        << "\"board_x\":" << config.board_x << ","
        << "\"board_y\":" << config.board_y << ","
        << "\"ai_runs\":" << config.ai_runs << ","

        << "\"alpha\":" << config.alpha << ","
        << "\"gamma\":" << config.gamma << ","
        << "\"epsilon\":" << config.epsilon << ","
        << "\"epsilon_decay\":" << config.epsilon_decay << ","
        << "\"epsilon_min\":" << config.epsilon_min << ","
        << "\"statefn\":\"" << State::serialise(config.stateFn) << "\","
        << "\"state_init\":\"" << State::serialise(config.stateInit) << "\","

        << "\"reward_advance\":" << config.reward_advance << ","
        << "\"reward_green\":" << config.reward_green << ","
        << "\"reward_red\":" << config.reward_red << ","
        << "\"reward_death\":" << config.reward_death
        << "}"
    ;
    return oss.str();
}