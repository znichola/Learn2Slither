#pragma once

#include <optional>
#include <string>

#include "trainer.hpp"

namespace AgentIO {

#ifndef __EMSCRIPTEN__

bool save(const std::string &path, const std::string &content);
std::optional<std::string> load(const std::string &path);

#endif

}
