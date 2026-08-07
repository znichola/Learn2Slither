#include <iostream>
#include <sstream>
#include <random>

#include "environment.hpp"
#include "interpreter.hpp"
#include "logger.hpp"
#include "reader.hpp"
#include "trainer.hpp"
#include "configParser.hpp"
#include "agentIO.hpp"

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

    state.trainer = Trainer(config);
    state.trainer.train();
}

static void handle(const Reader::ResumeTrain& m, AppState& state) {
    if (m.content != "\"continue\"") {
        Trainer::Config config = parseConfig(m.content);
        state.trainer.updateConfig(config);
    }

    state.trainer.train();
}

static void handle(const Reader::AI& m, AppState& state) {
    state.trainer.agent.epsilon = std::stof(m.content);
    Logger::log() << "Starting AI play mode, with latest trained model, with an exploration rate of " 
        << state.trainer.agent.epsilon << " (uses epsilonMin)";
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
    if (m.content == "Toggle snake vision") {
        Board::toggleSnakeVision();
        Logger::board() << state.board;
        return ;
    }

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

    // Logger::log() << "Move command " << m.content << ", result " << state.board.moveRes;
    Logger::board() << state.board;
    if (state.state == AppState::GS::GameOver) {
        Logger::RUN_DONE() << "GameOver | length " << state.board.snakeLength();
    }
}

static void handle(const Reader::SaveAgent& m, AppState& state) {
    Logger::log() << "Saved AGENT_" << state.trainer.agent.rng()
                  << " | State representation: " << State::serialise(state.trainer.config.stateFn)
                  << " | Q-table entries: " << state.trainer.agent.q_table.size();

    std::stringstream ss;
    ss << "AGENT_" << state.trainer.agent.rng() << "\n\n"
        << "CONFIG\n" << serialiseConfig(state.trainer.config) << "\n\n"
        << "QTABLE\n" << state.trainer.agent.serialiseQTable();

#ifndef __EMSCRIPTEN__
    if (!AgentIO::save(m.content, ss.str())) {
        Logger::error() << "Save error: could not open file for writing: " << m.content;
    } else {
        Logger::log() << "Save learning state in " << m.content;
    }
#else
    (void)m;
    Logger::save_agent() << ss.str();
#endif
}

static void handle(const Reader::LoadAgent& m, AppState& state) {
#ifndef __EMSCRIPTEN__
    auto c = AgentIO::load(m.content);
    if (!c.has_value()) {
        Logger::error() << "Load error: could not open file: " << m.content;
        return;
    }
    std::string content = c.value();
    Logger::log() << "Load trained model from " << m.content;
#else
    Logger::log() << "Loading agent ...";
    std::string content = m.content;
#endif

    const auto agentPos = content.find("AGENT_");
    const auto configPos = content.find("CONFIG\n");
    const auto qtablePos = content.find("QTABLE\n");

    if (agentPos == std::string::npos || agentPos > configPos || agentPos > qtablePos) {
        Logger::error() << "Load error: missing sections, agent:" << agentPos << " config:" << configPos << " qtable:" << qtablePos;
        return;
    }

    const std::string agentName = content.substr(agentPos, configPos - agentPos);
    const std::string agentConfig = content.substr(configPos + 7, qtablePos - (configPos + 7));
    const std::string agentQtable = content.substr(qtablePos + 7);

    state.trainer.config = parseConfig(agentConfig);
    state.trainer.agent.parseQTable(agentQtable);
    state.trainer.agent.epsilon = state.trainer.config.epsilon_min;

    Logger::log() << "Loaded " << agentName
                  << " | State representation: " << State::serialise(state.trainer.config.stateFn)
                  << " | Q-table entries: " << state.trainer.agent.q_table.size();

    Logger::QTABLE_SIZE(state.trainer.agent.q_table.size());
    Logger::PUSH_CONFIG() << serialiseConfig(state.trainer.config);
}

AppState initState() {
    Trainer trainer(Trainer::Config{});
    
    std::mt19937 rng(42);
    
    auto board = trainer.pipe.genBoard(rng());

    return AppState{board, 42, rng, AppState::GS::Playing, trainer};
}
