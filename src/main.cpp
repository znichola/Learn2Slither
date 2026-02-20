#include <iostream>
#include <sstream>
#include <random>

#include "environment.hpp"
#include "interpreter.hpp"
#include "logger.hpp"
#include "reader.hpp"
#include "trainer.hpp"
#include "configParser.hpp"

struct AppState {
    enum class GS { Playing, GameOver };

    Board board;
    int seed;
    std::mt19937 rng;

    GS state;

    Trainer trainer;
};

static void handle(const Reader::Start& m, AppState& state);
static void handle(const Reader::Step& m, AppState& state);
static void handle(const Reader::Train& m, AppState& state);
static void handle(const Reader::ResumeTrain& m, AppState& state);

static void dispatch(const Reader::Message& msg, AppState& state) {
    std::visit([&](auto&& m) { handle(m, state); }, msg);
}

AppState initState();

int main() {

    AppState state = initState();
    
    Reader::Parser parser;

    const auto drain = [&]() {
        while (auto msg = parser.pop()) dispatch(*msg, state);
    };

    while (parser.read()) drain();
    drain();

    Logger::log() << "Passed the blocking read in C++";
}

static void handle(const Reader::Train& m, AppState& state) {
    (void)state;

    Logger::log() << "Got a train command " << m.content;

    Trainer::Config config = parseConfig(m.content);

    Logger::log() << "Parsed config: " 
        << "EPISODES=" << config.EPISODES << "\n"
        << ", batch_size=" << config.BATCH_SIZE
        << ", frame_time_ms=" << config.frame_time_ms
        << ", MAX_STEPS=" << config.MAX_STEPS
        << ", board_x=" << config.board_x
        << ", board_y=" << config.board_y
        << ", alpha=" << config.alpha
        << ", gamma=" << config.gamma
        << ", epsilon=" << config.epsilon
        << ", epsilon_decay=" << config.epsilon_decay
        << ", epsilon_min=" << config.epsilon_min
        << ", reward_advance=" << config.reward_advance
        << ", reward_green=" << config.reward_green
        << ", reward_red=" << config.reward_red
        << ", reward_death=" << config.reward_death
        ;

    state.trainer =  Trainer(config);
    state.trainer.train();
}

static void handle(const Reader::ResumeTrain& m, AppState& state) {
    (void)m;
    state.trainer.train();
}

AppState initState() {
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
    return AppState{board, seed, rng, AppState::GS::Playing, Trainer(Trainer::Config{})};
}

static void handle(const Reader::Start& m, AppState& state) {
    (void)m;

    std::initializer_list<Move> moves = { Move::Down, Move::Down, Move::Left, Move::Left, Move::Left, Move::Down, Move::Left, Move::Left, Move::Up, Move::Left, Move::Left};
    
    Logger::log() << "Game initialized with seed: " << state.seed;
    Logger::board() << state.board;

    state.state = AppState::GS::Playing;

    return ;

    int i = 1;
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        auto move = *it;
        Board newBoard = state.board.doMove(move, state.rng());
        state.board = newBoard;

        Logger::log() << "Move " << i << ": " << move << " - Result: " << newBoard.moveRes;
        Logger::board() << state.board;

        if (newBoard.moveRes == MoveRes::Death) {
            Logger::error() << "Game Over: Snake died!";
            break;
        }
        i++;
    }
}

static void handle(const Reader::Step& m, AppState& state) {
    Logger::log() << "Got a move command " << m.content;

    if (state.state == AppState::GS::GameOver) {
        Logger::error() << "Sorry game over! restart if you want";
        return ;
    }

    Move move;
    if (m.content == "UP") {
        move = Move::Up;
    } else if (m.content == "DOWN") {
        move = Move::Down;
    } else if (m.content == "LEFT") {
        move = Move::Left;
    } else if (m.content == "RIGHT") {
        move = Move::Right;
    } else {
        Logger::error() << "Unknown move: " << m.content;
        return;
    }

    Board newBoard = state.board.doMove(move, state.rng());

    state.board = newBoard;

    if (newBoard.moveRes == MoveRes::Death) {
        state.state = AppState::GS::GameOver;
    }


    Logger::log() << "Result: " << newBoard.moveRes;
    Logger::board() << state.board;
}
