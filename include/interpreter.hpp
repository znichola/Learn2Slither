#pragma once

#include <iostream>
#include <algorithm>
#include <cstdint>
#include <functional>

#include "environment.hpp"

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

namespace State {
    static char map(Board::Cell c) {
    return Board::mapping[static_cast<size_t>(c)];
}
    typedef std::function<std::string(const Vision&)> StateFn;

    inline std::string full(const Vision &v) {
        std::string res;
        res.reserve(36);
        for (auto& c : v._north) res += map(c);
        for (auto& c : v._east)  res += map(c);
        for (auto& c : v._south) res += map(c);
        for (auto& c : v._west)  res += map(c);
        return res;
    }

    inline std::string firstNonEmpty(const Vision &v) {
        std::string res;
        res.reserve(4);

        auto compress = [&](const auto& direction) {
            for (auto& c : direction) {
                if (c == Board::Cell::Empty) continue;
                res += map(c);
                break;
            }
        };

        compress(v._north);
        compress(v._east);
        compress(v._south);
        compress(v._west);

        return res;
    }

    inline std::string firstAndNextNonEmpty(const Vision &v) {
        std::string res;
        res.reserve(8);

        auto compress = [&](const auto& direction) {
            bool first = true;
            for (auto& c : direction) {
                if (first) {
                    res += map(c);
                    first = false;
                } else {
                    if (c == Board::Cell::Empty) continue;
                    res += map(c);
                    break;
                }
            }
        };

        compress(v._north);
        compress(v._east);
        compress(v._south);
        compress(v._west);

        return res;
    }

    enum class Type { FULL, FIRST_NON_EMPTY, FIRST_AND_NEXT_NON_EMPTY };

    inline std::string serialise(Type t) {
        switch (t) {
            case Type::FULL: return "FULL";
            case Type::FIRST_NON_EMPTY: return "FIRST_NON_EMPTY";
            case Type::FIRST_AND_NEXT_NON_EMPTY: return "FIRST_AND_NEXT_NON_EMPTY";
            default: return "UNKNOWN";
        }
    }

    inline Type parse(const std::string& s) {
        std::cout << "PARSING " << s << "\n";
        if (s == "FULL") return Type::FULL;
        if (s == "FIRST_NON_EMPTY") return Type::FIRST_NON_EMPTY;
        if (s == "FIRST_AND_NEXT_NON_EMPTY") return Type::FIRST_AND_NEXT_NON_EMPTY;

        throw std::invalid_argument("Invalid State::Type string: " + s);
    }

    inline StateFn get(Type t) {
        switch (t) {
            case Type::FULL: return full;
            case Type::FIRST_NON_EMPTY: return firstNonEmpty;
            case Type::FIRST_AND_NEXT_NON_EMPTY: return firstAndNextNonEmpty;
        }
        throw std::runtime_error("Invalid STATE representation");
    }


}

/*
class State {
public:
    std::string value;

    State(const Vision& vision) {
        value.reserve(36);
        for (auto& c : vision._north) value += map(c);
        for (auto& c : vision._east)  value += map(c);
        for (auto& c : vision._south) value += map(c);
        for (auto& c : vision._west)  value += map(c);
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
*/

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