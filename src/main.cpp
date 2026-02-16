#include <iostream>
#include <sstream>
#include <random>

#include "environment.hpp"
#include "interpreter.hpp"
#include "logger.hpp"

int main() {
    Logger logger(LoggerConfig{});

    auto board = Board(R"(
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
    int seed = 42;

    std::mt19937 rng(seed);

    auto moves = { Move::Down, Move::Down, Move::Left, Move::Left, Move::Left, Move::Down, Move::Left, Move::Left, Move::Up, Move::Left, Move::Left};

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

