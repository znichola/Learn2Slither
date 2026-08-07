#pragma once

#include <ostream>

enum class Move {Up=0, Right=1, Down=2, Left=3};

inline std::ostream& operator<<(std::ostream& os, const Move m) {
    switch (m) {
        case Move::Up:    os << "UP";    break;
        case Move::Right: os << "RIGHT"; break;
        case Move::Down:  os << "DOWN";  break;
        case Move::Left:  os << "LEFT";  break;
    }
    return os;
}

enum class MoveRes {Death, Red, Green, Advance};

inline std::ostream& operator<<(std::ostream& os, const MoveRes m) {
    switch (m) {
        case MoveRes::Death:   os << "Death";           break;
        case MoveRes::Red:     os << "Ate Red Apple";   break;
        case MoveRes::Green:   os << "Ate Green Apple"; break;
        case MoveRes::Advance: os << "Advanced";        break;
    }
    return os;
}
