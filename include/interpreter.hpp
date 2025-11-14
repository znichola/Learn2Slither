#pragma once

#include <iostream>

#include "environment.hpp"

class Vision {
public:
    std::vector<Board::Cell> _north;
    std::vector<Board::Cell> _east;
    std::vector<Board::Cell> _south;
    std::vector<Board::Cell> _west;

    inline Vision(const Board &board) {
        auto head_it = std::find(
                board._grid.begin(), board._grid.end(), Board::Cell::Head);
        auto rhead_it = std::find(
                board._grid.rbegin(), board._grid.rend(), Board::Cell::Head);
        auto c = std::count(
                board._grid.begin(), board._grid.end(), Board::Cell::Head);
        if (c != 1 || head_it == board._grid.end()
                || rhead_it == board._grid.rend()) {
            throw std::runtime_error("Vision: board must contain one head");
        }

        auto east_it = find(head_it, board._grid.end(), Board::Cell::Wall);
        _east = std::vector<Board::Cell>(head_it+1, east_it+1);

        auto rwest_it = find(rhead_it, board._grid.rend(), Board::Cell::Wall);
        _west = std::vector<Board::Cell>(rhead_it+1, rwest_it+1);

        unsigned step = board.x_dim;
        for (auto i = head_it + step;
                i<board._grid.end(); std::advance(i, step)) {
            _south.push_back(*i);
        }

        for (auto i = rhead_it + step;
               i<board._grid.rend(); std::advance(i, step)) {
            _north.push_back(*i);
        }
    }
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

class Action {
public:
    enum class Move {Up, Down, Left, Right};

    // Action constructor will take as atgument the Q function?
    // or whatever huristic is being used

    Move move(const Vision &vision) const;
};

inline std::ostream& operator<<(std::ostream& os, const Action::Move m) {
    switch (m) {
        case Action::Move::Up:    os << "UP";    break;
        case Action::Move::Down:  os << "DOWN";  break;
        case Action::Move::Left:  os << "LEFT";  break;
        case Action::Move::Right: os << "RIGHT"; break;
    }
    return os;
}

void loop();

/* The Agent makes actions, which produce moves, these moves are fed into a 
 * 
 * */

