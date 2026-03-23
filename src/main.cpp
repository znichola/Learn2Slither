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

static void handle(const Reader::Manual& m, AppState& state);
static void handle(const Reader::Step& m, AppState& state);
static void handle(const Reader::Train& m, AppState& state);
static void handle(const Reader::ResumeTrain& m, AppState& state);
static void handle(const Reader::AI& m, AppState& state);

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
    Trainer::Config config = parseConfig(m.content);

    Logger::log() << "Parsed config: " 
        << "EPISODES=" << config.EPISODES << "\n"
        << ", SAMPLE_PER_REPLAY=" << config.SAMPLE_PER_REPLAY
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

    state.trainer = Trainer(config);
    state.trainer.train();
}

static void handle(const Reader::ResumeTrain& m, AppState& state) {
    (void)m;
    state.trainer.train();
}

static void handle(const Reader::AI& m, AppState& state) {
    (void)m;
    Logger::log() << "Starting AI play mode, with latest trained model";
    state.trainer.AIplay();
}

static void handle(const Reader::Manual& m, AppState& state) {
    Logger::log() << "Start playing, use buttons, arrow keys, wasd or vim motions!";

    Trainer::Config config = parseConfig(m.content);
    state.trainer = Trainer(config);
    state.board = state.trainer.pipe.genBoard(state.rng());

    if (state.state == AppState::GS::GameOver) {
        Logger::log() << "Restarting the game with a new random board";
        state.board = state.trainer.pipe.genBoard(state.rng());
    }

    Logger::board() << state.board;
    state.state = AppState::GS::Playing;
    return ;
}


static void handle(const Reader::Step& m, AppState& state) {
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

    state.board = state.board.doMove(move, state.rng());

    if (state.board.moveRes == MoveRes::Death) {
        state.state = AppState::GS::GameOver;
    }

    Logger::log() << "Move command " << m.content << ", result " << state.board.moveRes;
    Logger::board() << state.board;
}

AppState initState() {
    Trainer trainer(Trainer::Config{});
    
    std::mt19937 rng(42);
    
    auto board = trainer.pipe.genBoard(rng());

    return AppState{board, 42, rng, AppState::GS::Playing, trainer};
}
