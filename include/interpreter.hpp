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

inline std::ostream& operator<<(std::ostream& os, const Vision &v) {
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

    enum class Type { FULL, FIRST_NON_EMPTY, FIRST_AND_NEXT_NON_EMPTY }; // SINGLE_DIMENTION

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

inline std::ostream& operator<<(std::ostream& os, const Board& b) {
    if (b.snakeVision) {
        os << Vision(b);
    } else {
        int i = 0;
        for (const auto cell : b._grid) {
            if (cell == Board::Cell::Empty)
                os << " ";
            else
                os << cell;
            if ((i + 1) % b.x_dim == 0) {
                os << "\n";
            }
            i++;
        }
    }
    return os;
}
