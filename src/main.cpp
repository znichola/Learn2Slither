#include <iostream>
#include <sstream>
#include <random>
#include <unordered_map>

#include "environment.hpp"
#include "interpreter.hpp"
#include "logger.hpp"
#include "reader.hpp"

struct GameState {
    enum class GS { Playing, GameOver };

    Board board;
    int seed;
    std::mt19937 rng;

    GS state;

};

static void handle(const Reader::Start& m, GameState& state);
static void handle(const Reader::Step& m, GameState& state);

static void dispatch(const Reader::Message& msg, GameState& state) {
    std::visit([&](auto&& m) { handle(m, state); }, msg);
}

GameState initState();

int main() {

    GameState state = initState();
    
    Reader::Parser parser;

    const auto drain = [&]() {
        while (auto msg = parser.pop()) dispatch(*msg, state);
    };

    while (parser.read()) drain();
    drain();

    Logger::log() << "Passed the blocking read in C++";
}

GameState initState() {
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
    return GameState{board, seed, rng, GameState::GS::Playing };
}

static void handle(const Reader::Start& m, GameState& state) {
    (void)m;

    std::initializer_list<Move> moves = { Move::Down, Move::Down, Move::Left, Move::Left, Move::Left, Move::Down, Move::Left, Move::Left, Move::Up, Move::Left, Move::Left};

    Logger::log() << "Game initialized with seed: " << state.seed;
    Logger::board() << state.board;
    
    return ;

    int i = 1;
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        auto move = *it;
        auto [newBoard, moveRes] = state.board.doMove(move, state.rng());
        state.board = newBoard;

        Logger::log() << "Move " << i << ": " << move << " - Result: " << moveRes;
        Logger::board() << state.board;

        if (moveRes == MoveRes::Death) {
            Logger::error() << "Game Over: Snake died!";
            break;
        }
        i++;
    }
}

static void handle(const Reader::Step& m, GameState& state) {
    Logger::log() << "Got a move command " << m.content;

    if (state.state == GameState::GS::GameOver) {
        Logger::log() << "Sorry game over! restart if you want";
        return ;
    }

    static const std::unordered_map<std::string, Move> map{
        {"UP", Move::Up},
        {"DOWN", Move::Down},
        {"LEFT", Move::Left},
        {"RIGHT", Move::Right}
    };

    auto it = map.find(m.content);
    if (it == map.end()) {
        Logger::error() << "Unknown move: " << m.content;
        return;
    }

    auto [newBoard, moveRes] = state.board.doMove(it->second, state.rng());

    state.board = newBoard;

    if (moveRes == MoveRes::Death) {
        state.state = GameState::GS::Playing;
    }

    Logger::log() << "Result: " << moveRes;
    Logger::board() << state.board;
}
