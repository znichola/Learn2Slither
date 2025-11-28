#pragma once

#include <iostream>
#include <algorithm>

#include "environment.hpp"

float reward(MoveRes);

void loop();

class Vision {
public:
    std::vector<Board::Cell> _north;
    std::vector<Board::Cell> _east;
    std::vector<Board::Cell> _south;
    std::vector<Board::Cell> _west;

    Vision(const Board &board);
};

inline std::ostream& operator<<(std::ostream& os, const Vision v) {
    for (auto i = v._north.rbegin(); i < v._north.rend(); i++) {
        os << std::string(v._west.size(), ' ')
           << *i
           << std::string(v._east.size(), ' ')
           << "\n";
    }
    for (auto i = v._west.rbegin(); i < v._west.rend(); i++) {
        os << *i;
    }
    os << Board::Cell::Head;
    for (auto i = v._east.begin(); i < v._east.end(); i++) {
        os << *i;
    }
    os << "\n";
    for (auto i = v._south.begin(); i < v._south.end(); i++) {
        os << std::string(v._west.size(), ' ')
           << *i
           << std::string(v._east.size(), ' ')
           << "\n";
    }

    if (false) {
        os << "NORTH: ";
        for (auto c : v._north) os << c;
        os << "\nEast: ";
        for (auto c : v._east) os << c;
        os << "\nSOUTH: ";
        for (auto c : v._south) os << c;
        os << "\nWEST: ";
        for (auto c : v._west) os << c;
        os << "\n";
    }
    return os;
}

class State {
public:
    uint64_t value = 0;  // packed 60 bits

    State(const Vision &vision);

    inline bool operator==(const State& other) const {
        return value == other.value;
    }

    // Hash functor for std::unordered_map
    struct Hash {
        inline std::size_t operator()(const State& s) const noexcept {
            return s.value;
        }
    };
};

