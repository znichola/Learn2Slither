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
    typedef std::function<std::string(const std::vector<Board::Cell>&)> RayStateFn;

    inline std::string rayFull(const std::vector<Board::Cell>& ray) {
        std::string res;
        res.reserve(ray.size());

        for (auto& c : ray) {
            res += map(c);
        }
        return res;
    }

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

    inline std::string rayFirstAndNextNonEmpty(const std::vector<Board::Cell>& ray) {
        std::string res;
        res.reserve(2);

        bool first = true;
        for (auto& c : ray) {
            if (first) {
                res += map(c);
                first = false;
            } else {
                if (c == Board::Cell::Empty) continue;
                res += map(c);
                break;
            }
        }
        if (res.size() == 1) {
            res += res[0];
        }
        return res;
    }

    inline std::string firstAndNextNonEmpty(const Vision &v) {
        std::string res;
        res.reserve(2*4);
        res += rayFirstAndNextNonEmpty(v._north);
        res += rayFirstAndNextNonEmpty(v._east);
        res += rayFirstAndNextNonEmpty(v._south);
        res += rayFirstAndNextNonEmpty(v._west);
        return res;
    }

    inline std::string rayDistToClosest(const std::vector<Board::Cell>& ray) {
        std::string res;
        res.reserve(3);

        size_t firstIdx = ray.size();
        for (size_t i = 0; i < ray.size(); ++i) {
            if (ray[i] != Board::Cell::Empty) {
                firstIdx = i;
                break;
            }
        }

        if (firstIdx == ray.size()) {
            res += '4';
            res += Board::mapping[static_cast<int>(Board::Cell::Empty)];
            res += Board::mapping[static_cast<int>(Board::Cell::Empty)];
            return res;
        }

        int dist = static_cast<int>(firstIdx) + 1;
        if (dist > 4) dist = 4;
        res += static_cast<char>('0' + dist);
        res += Board::mapping[static_cast<int>(ray[firstIdx])];

        size_t nextIdx = ray.size();
        for (size_t i = firstIdx + 1; i < ray.size(); ++i) {
            if (ray[i] != Board::Cell::Empty) {
                nextIdx = i;
                break;
            }
        }

        res += (nextIdx != ray.size())
            ? Board::mapping[static_cast<int>(ray[nextIdx])]
            : Board::mapping[static_cast<int>(ray[firstIdx])];

        return res;
    }

    inline std::string distToClosest(const Vision &v) {
        std::string res;
        res.reserve(3*4);
        res += rayDistToClosest(v._north);
        res += rayDistToClosest(v._east);
        res += rayDistToClosest(v._south);
        res += rayDistToClosest(v._west);
        return res;
    }


    enum class Type { FULL, FIRST_NON_EMPTY, FIRST_AND_NEXT_NON_EMPTY, DIST_TO_CLOSEST, SINGLE_DIMENSION_FULL, SINGLE_DIMENSION_FANNE, SINGLE_DIMENSION_DTC };
    enum class Init { ZEROS, INSTANT_DEATH };

    inline std::string serialise(Type t) {
        switch (t) {
            case Type::FULL: return "FULL";
            case Type::FIRST_NON_EMPTY: return "FIRST_NON_EMPTY";
            case Type::FIRST_AND_NEXT_NON_EMPTY: return "FIRST_AND_NEXT_NON_EMPTY";
            case Type::DIST_TO_CLOSEST: return "DIST_TO_CLOSEST";
            case Type::SINGLE_DIMENSION_FULL: return "SINGLE_DIMENSION_FULL";
            case Type::SINGLE_DIMENSION_FANNE: return "SINGLE_DIMENSION_FANNE";
            case Type::SINGLE_DIMENSION_DTC: return "SINGLE_DIMENSION_DTC";
            default: return "UNKNOWN";
        }
    }

    inline std::string serialise(Init i) {
        switch (i) {
            case Init::ZEROS: return "ZEROS";
            case Init::INSTANT_DEATH: return "INSTANT_DEATH";
            default: return "UNKNOWN";
        }
    }

    inline Type parse(const std::string& s) {
        if (s == "FULL") return Type::FULL;
        if (s == "FIRST_NON_EMPTY") return Type::FIRST_NON_EMPTY;
        if (s == "FIRST_AND_NEXT_NON_EMPTY") return Type::FIRST_AND_NEXT_NON_EMPTY;
        if (s == "DIST_TO_CLOSEST") return Type::DIST_TO_CLOSEST;
        if (s == "SINGLE_DIMENSION_FANNE") return Type::SINGLE_DIMENSION_FANNE;
        if (s == "SINGLE_DIMENSION_FULL") return Type::SINGLE_DIMENSION_FULL;
        if (s == "SINGLE_DIMENSION_DTC") return Type::SINGLE_DIMENSION_DTC;

        throw std::invalid_argument("Invalid State::Type string: " + s);
    }

    inline Init parseInit(const std::string& s) {
        std::cout << "PARSING INIT " << s << "\n";
        if (s == "ZEROS") return Init::ZEROS;
        if (s == "INSTANT_DEATH") return Init::INSTANT_DEATH;

        throw std::invalid_argument("Invalid State::Init string: " + s);
    }

    inline StateFn get(Type t) {
        switch (t) {
            case Type::FULL: return full;
            case Type::FIRST_NON_EMPTY: return firstNonEmpty;
            case Type::FIRST_AND_NEXT_NON_EMPTY: return firstAndNextNonEmpty;
            case Type::DIST_TO_CLOSEST: return distToClosest;
            case Type::SINGLE_DIMENSION_FANNE: return firstAndNextNonEmpty; // invalid as it's superseded by getRay
            case Type::SINGLE_DIMENSION_FULL: return full; // invalid as it's superseded by getRay
            case Type::SINGLE_DIMENSION_DTC: return distToClosest; // invalid as it's superseded by getRay
        }
        throw std::runtime_error("Invalid STATE representation");
    }

    inline RayStateFn getRay(Type t) {
        switch (t) {
            case Type::SINGLE_DIMENSION_FANNE: return rayFirstAndNextNonEmpty;
            case Type::SINGLE_DIMENSION_FULL: return rayFull;
            case Type::SINGLE_DIMENSION_DTC: return rayDistToClosest;
            default: return nullptr;
        }
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