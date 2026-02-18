#include <iostream>
#include <sstream>
#include <random>

#include "environment.hpp"
#include "interpreter.hpp"
#include "logger.hpp"
#include "reader.hpp"

static void handle(const Reader::Start&     m);
static void handle(const Reader::Send& m) { (void)m; }

static void dispatch(const Reader::Message& msg) {
    std::visit([](auto&& m) { handle(m); }, msg);
}

int main() {
    Reader::Parser parser;

    const auto drain = [&]() {
        while (auto msg = parser.pop()) dispatch(*msg);
    };

    while (parser.read()) drain();
    drain();
    Logger::log() << "Passed the blocking read in C++";
}

static void handle(const Reader::Start& m) {
    Logger::error() << m.content;
    
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

    Logger::log() << "Game initialized with seed: " << seed;
    Logger::board() << board;
    int i = 1;
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        auto move = *it;
        auto [newBoard, moveRes] = board.doMove(move, rng());
        board = newBoard;

        Logger::log() << "Move " << i << ": " << move << " - Result: " << moveRes;
        Logger::board() << board;

        if (moveRes == MoveRes::Death) {
            Logger::error() << "Game Over: Snake died!";
            break;
        }
        i++;
    }
}