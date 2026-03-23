#pragma once

#include <iostream>
#include <algorithm>
#include <cstdint>

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
    std::string value;

    State(const Vision& vision) {
        value.reserve(36);
        for (auto& c : vision._north) value += static_cast<char>(c);
        for (auto& c : vision._east)  value += static_cast<char>(c);
        for (auto& c : vision._south) value += static_cast<char>(c);
        for (auto& c : vision._west)  value += static_cast<char>(c);
    }

    bool operator==(const State& other) const {
        return value == other.value;
    }

    struct Hash {
        std::size_t operator()(const State& s) const noexcept {
            return std::hash<std::string>{}(s.value);
        }
    };
};

/*

class State {
public:
    std::array<uint64_t, 2> value{};

    State(const Vision& vision) {
        auto encode = [&](const std::vector<Board::Cell>& arm, int arm_idx) {
            for (int i = 0; i < (int)arm.size(); ++i)
                set(arm_idx * 9 + i, arm[i]);
        };
        encode(vision._north, 0);
        encode(vision._east,  1);
        encode(vision._south, 2);
        encode(vision._west,  3);
    }

    bool operator==(const State& other) const {
        return value == other.value;
    }

    struct Hash {
        std::size_t operator()(const State& s) const noexcept {
            // Multiply w[0] by a large odd constant before XOR-ing w[1].
            // This breaks symmetry — without it, swapped word pairs would
            // hash identically and cluster in the same buckets.
            return s.value[0] * 2654435761ULL ^ s.value[1];
        }
    };

private:
    void set(int i, Board::Cell cell) {
        // 36 tiles × 3 bits each = 108 bits total.
        // Split across two uint64_t: word 0 holds tiles 0–20 (63 bits used),
        // word 1 holds tiles 21–35 (45 bits used).
        int wi = i / 21;       // which word (0 or 1)
        int b  = (i % 21) * 3; // bit offset within that word

        // Clear the 3-bit slot, then write the new value into it.
        // ~(7ULL << b) is a mask with 000 at position b and 1s everywhere else.
        value[wi] = (value[wi] & ~(7ULL << b)) | (uint64_t(cell) << b);
    }
};

*/