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
static void handle(const Reader::SaveAgent &m, AppState& state);
static void handle(const Reader::LoadAgent &m, AppState& state);

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

    Logger::log() << "Parsed config:\n" << serialiseConfig(config);

    // TODO need a seperate start a fresh training and continue training (using existing agent state)

    state.trainer = Trainer(config);
    state.trainer.train();
}

static void handle(const Reader::ResumeTrain& m, AppState& state) {
    (void)m;
    state.trainer.train();
    // TODO this feels bugged as all heck. Should be looked at
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

static void handle(const Reader::SaveAgent& m, AppState& state) {
    (void)m;
    Logger::log() << "Saving and sending agent state ...";

    Logger::log() << "Saved AGENT_" << state.trainer.agent.rng()
                  << " | State representation: " << State::serialise(state.trainer.config.stateFn)
                  << " | Q-table entries: " << state.trainer.agent.q_table.size();

    Logger::save_agent() << "AGENT_" << state.trainer.agent.rng() << "\n\n"
            << "CONFIG\n" << serialiseConfig(state.trainer.config) << "\n\n"
            << "QTABLE\n" << state.trainer.agent.serialiseQTable();
}

static void handle(const Reader::LoadAgent& m, AppState& state) {
    (void)state;
    Logger::log() << "Loading agent ...";
    auto agentPos = m.content.find("AGENT_");
    auto configPos = m.content.find("CONFIG\n");
    auto qtablePos = m.content.find("QTABLE\n");

    if (agentPos == std::string::npos || agentPos > configPos || agentPos > qtablePos) {
        Logger::error() << "Load error: missing sections, agent:" << agentPos << " config:" << configPos << " qtable:" << qtablePos;
        return;
    }

    const std::string agentName = m.content.substr(agentPos, configPos - agentPos);
    const std::string agentConfig = m.content.substr(configPos + 7, qtablePos - (configPos + 7));
    const std::string agentQtable = m.content.substr(qtablePos + 7);

    state.trainer.config = parseConfig(agentConfig);
    state.trainer.agent.parseQTable(agentQtable);

    Logger::log() << "Loaded " << agentName
                  << "| State representation: " << State::serialise(state.trainer.config.stateFn)
                  << "| Q-table entries: " << state.trainer.agent.q_table.size();

    // TODO send the update config message for the UI
}

AppState initState() {
    Trainer trainer(Trainer::Config{});
    
    std::mt19937 rng(42);
    
    auto board = trainer.pipe.genBoard(rng());

    return AppState{board, 42, rng, AppState::GS::Playing, trainer};
}
