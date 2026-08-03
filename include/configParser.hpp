#pragma once

#include "trainer.hpp"

Trainer::Config parseConfig(const std::string &configStr);

std::string serialiseConfig(const Trainer::Config &config);