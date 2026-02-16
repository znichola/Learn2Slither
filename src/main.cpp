#include <iostream>
#include <sstream>
#include <random>

#include "environment.hpp"
#include "interpreter.hpp"
#include "logger.hpp"

int main() {
    Logger logger(LoggerConfig{});

    auto foo = Board(R"(
WWWWWWWWWWWW
W          W
W          W
W          W
W       SSHW
W          W
W   G R    W
W          W
W          W
W          W
W        G W
WWWWWWWWWWWW
            )");
    std::cout << foo;
    for (auto s : foo._snake) {
        std::cout << s << " ";
    }
    std::cout << "\n";


    Pipe p(Board(10, 10), {
            Pipe::RandomSpawn{Board::Cell::Head},
            Pipe::RandomConnectedSpawn{Board::Cell::Head,
                Board::Cell::Snake, 2},
            Pipe::RandomSpawn{Board::Cell::Red},
            Pipe::RandomSpawn{Board::Cell::Green},
            Pipe::RandomSpawn{Board::Cell::Green},
            });
    auto seed = 42;
    std::mt19937 rng(seed);
    auto board = p.genBoard(seed);

    std::cout << board;
    for (auto s : board._snake) {
        std::cout << s << " ";
    }
    std::cout << "\n";
    // return 0;

    auto moves = { Move::Up, Move::Down, Move::Left, Move::Left, Move::Left, Move::Left, Move::Left, Move::Left, Move::Left };

    logger.log() << "Game initialized with seed: " << seed;
    logger.board() << board;
    int i = 1;
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        auto move = *it;
        auto [newBoard, moveRes] = board.doMove(move, rng());
        board = newBoard;

        logger.log() << "Move " << i << ": " << move << " - Result: " << moveRes;
        logger.board() << board;

        if (moveRes == MoveRes::Death) {
            logger.error() << "Game Over: Snake died!";
            break;
        }
        i++;
    }
}

