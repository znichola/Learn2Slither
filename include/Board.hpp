#pragma once

#include <vector>
#include <ostream>
#include <utility>
#include <variant>

class Board {
public:
    enum class Cell {Empty, Wall, Head, Snake, Green, Red};
    typedef std::pair<Board, unsigned> Op;

    unsigned int x_dim = 10;
    unsigned int y_dim = 10;
    std::vector<Cell> grid;

    Board(unsigned int x, unsigned int y);

    // Board& operator=(Board &b) const = default;

    ~Board() = default;

    // Spawns a cell of type t somewhere randomly on an Empty square
    // returns the new board and the next seed

     Op randomSpawn(Cell t, unsigned seed) const;
};

class Pipe {
public:
    struct RandomSpawn {
        Board::Cell t;
    };
    struct RandomChainedSpawn {
        Board::Cell t;
        unsigned length;
    };
    struct Op : std::variant<RandomSpawn, RandomChainedSpawn> {
        using std::variant<RandomSpawn, RandomChainedSpawn>::variant;
    };
    inline static Board pipe(
            const Board & board,
            unsigned seed,
            std::vector<Op> ops
            ) {
        Board b = board; // TODO : switch to using ref
        unsigned s = seed;
        for (const Op & op : ops) {
            if (std::holds_alternative<RandomSpawn>(op)) {
                const auto &_op = std::get<RandomSpawn>(op);
                auto [newBoard, newSeed] = b.randomSpawn(_op.t, s);
                b = newBoard;
                s = newSeed;
            }
        }
        return b;
    }
};


inline std::ostream& operator<<(std::ostream& os, const Board::Cell c) {
    switch (c) {
        case Board::Cell::Empty: os << "0"; break;
        case Board::Cell::Wall: os << "W"; break;
        case Board::Cell::Head: os << "H"; break;
        case Board::Cell::Snake: os << "S"; break;
        case Board::Cell::Green: os << "G"; break;
        case Board::Cell::Red: os << "R"; break;
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Board& b) {
    int i = 0;
    for (const auto cell : b.grid) {
        os << cell;
        if ((i + 1) % b.x_dim == 0) {
            os << "\n";
        }
        i++;
    }
    return os;
}
