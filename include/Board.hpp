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
    Op randomConnectedSpawn(
            Cell h, Cell t, unsigned length, unsigned seed) const;
};

class Pipe {
public:
    struct RandomSpawn {
        Board::Cell t;
    };
    struct RandomConnectedSpawn {
        Board::Cell h;
        Board::Cell t;
        unsigned length;
    };
    struct Op : std::variant<RandomSpawn, RandomConnectedSpawn> {
        using std::variant<RandomSpawn, RandomConnectedSpawn>::variant;
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
            } else if (std::holds_alternative<RandomConnectedSpawn>(op)) {
                const auto &_op = std::get<RandomConnectedSpawn>(op);
                auto [newBoard, newSeed] = b.randomConnectedSpawn(
                        _op.h, _op.t, _op.length, s);
                b = newBoard;
                s = newSeed;
            }
        }
        return b;
    }

    Board _board;
    std::vector<Op> _pipeline;

    inline Pipe(const Board & board, const std::vector<Op> &pipeline)
        : _board(board), _pipeline(pipeline) {}

    inline Board genBoard(unsigned seed) const {
        return pipe(_board, seed, _pipeline);
    }
};


inline std::ostream& operator<<(std::ostream& os, const Board::Cell c) {
    switch (c) {
        case Board::Cell::Empty: os << " "; break;
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
