#pragma once

#include <vector>
#include <ostream>
#include <utility>
#include <variant>

#include "move.hpp"

class Board {
public:
    static constexpr char mapping[] = {'0', 'W', 'S', 'G', 'R', 'H'};
    enum class Cell {Empty=0, Wall=1, Snake=2, Green=3, Red=4, Head=5};
    typedef std::pair<Board, unsigned> Op;

    unsigned int x_dim = 10;
    unsigned int y_dim = 10;

    MoveRes moveRes = MoveRes::Advance;

    std::vector<Cell> _grid;
    std::vector<unsigned> _snake;

    Board(unsigned x, unsigned y);
    Board(const std::string &board);

    // Board& operator=(Board &b) const = default;

    ~Board() = default;

    // Spawns a cell of type t somewhere randomly on an Empty square
    // returns the new board and the next seed

    Op randomSpawn(Cell t, unsigned seed) const;
    Op randomConnectedSpawn(
            Cell h, Cell t, unsigned length, unsigned seed, bool asSnake) const;

    Board doMove(Move m, unsigned seed) const;

    unsigned snakeLength() const;

    inline static bool snakeVision = false;
    static void toggleSnakeVision() {
        snakeVision = !snakeVision;
    }
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
    static Board pipe(const Board &board, unsigned seed, std::vector<Op> ops);

    Board _board;
    std::vector<Op> _pipeline;

    inline Pipe(const Board & board, const std::vector<Op> &pipeline)
        : _board(board), _pipeline(pipeline) {}

    inline Board genBoard(unsigned seed) const {
        return pipe(_board, seed, _pipeline);
    }

    static Pipe standardPipeline();
};


inline std::ostream& operator<<(std::ostream& os, const Board::Cell c) {
    return os << Board::mapping[static_cast<size_t>(c)];
}

