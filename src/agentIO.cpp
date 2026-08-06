#include "agentIO.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "configParser.hpp"
#include "interpreter.hpp"
#include "logger.hpp"

namespace AgentIO {

#ifndef __EMSCRIPTEN__

bool save(const std::string &path, const std::string &content) {
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs)
        return false;
    ofs << content;

    return true;
}

std::optional<std::string> load(const std::string &path) {
    std::ifstream ifs(path);
    if (!ifs)
        return std::nullopt;

    std::ostringstream ss;
    ss << ifs.rdbuf();

    return ss.str();
}

#endif // __EMSCRIPTEN__

}
