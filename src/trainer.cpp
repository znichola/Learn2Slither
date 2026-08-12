#include "trainer.hpp"
#include "environment.hpp"
#include "interpreter.hpp"
#include "agent.hpp"
#include "configParser.hpp"

#include "logger.hpp"
#include <iomanip>

Trainer::Trainer(const Config &config)
    : config(config), 
    agent(Agent{
        .alpha = config.alpha,
        .gamma = config.gamma,
        .epsilon = config.epsilon,
        .epsilon_decay = config.epsilon_decay,
        .epsilon_min = config.epsilon_min,
        .reward_death = config.reward_death,
        .stateInit = config.stateInit,
        .state = State::get(config.stateFn),
        .rayState = State::getRay(config.stateFn),
        .q_table = {}
        }),
      pipe(Board(config.board_x, config.board_y), {
        Pipe::RandomSpawn{Board::Cell::Head},
        Pipe::RandomConnectedSpawn{Board::Cell::Head, Board::Cell::Snake, 2},
        Pipe::RandomSpawn{Board::Cell::Red},
        Pipe::RandomSpawn{Board::Cell::Green},
        Pipe::RandomSpawn{Board::Cell::Green},
        }) {}

void Trainer::updateConfig(const Config &config) {
    const auto oldStateFn = this->config.stateFn;
    _current_ep = 0;
    this->config = config;

    if (oldStateFn != this->config.stateFn) {
        Logger::error() << "Refusing to change state function on an existing agent, it remains: " << State::serialise(oldStateFn);
        this->config.stateFn = oldStateFn;
    }

    agent.alpha         = this->config.alpha;
    agent.gamma         = this->config.gamma;
    agent.epsilon       = this->config.epsilon;
    agent.epsilon_decay = this->config.epsilon_decay;
    agent.epsilon_min   = this->config.epsilon_min;
    agent.reward_death  = this->config.reward_death;
    agent.stateInit     = this->config.stateInit;

    this->pipe = Pipe(Board(config.board_x, config.board_y), {
        Pipe::RandomSpawn{Board::Cell::Head},
        Pipe::RandomConnectedSpawn{Board::Cell::Head, Board::Cell::Snake, 2},
        Pipe::RandomSpawn{Board::Cell::Red},
        Pipe::RandomSpawn{Board::Cell::Green},
        Pipe::RandomSpawn{Board::Cell::Green},
        });
}

Trainer::EpisodeResult Trainer::playEpisodeNoUpdate() {
    Board board = pipe.genBoard(agent.rng());

    currentBoards.clear();
    currentBoards.push_back(board);

    unsigned step = 0;

    const int maxStepsSinceEat = config.MAX_STEPS / 10;
    int stepsSinceEat = 0;
    Move lastMove = Move::Up;
    EpisodeResult::EndReason reason = EpisodeResult::EndReason::MaxSteps;

    for (; step < config.MAX_STEPS; step++) {
        Vision vision(board);
        Move action = agent.chooseActionNoUpdate(vision);
        lastMove = action;

        Board next_board = board.doMove(action, agent.rng());

        if (next_board.moveRes == MoveRes::Death) {
            currentBoards.push_back(next_board);
            reason = EpisodeResult::EndReason::Death;
            break;
        }

        if (next_board.moveRes == MoveRes::Green || next_board.moveRes == MoveRes::Red)
            stepsSinceEat = 0;
        else
            stepsSinceEat++;

        board = next_board;
        currentBoards.push_back(board);

        if (stepsSinceEat >= maxStepsSinceEat) {
            reason = EpisodeResult::EndReason::NoFoodTimeout;
            break;
        }
    }

    return EpisodeResult{ step, board.snakeLength(), lastMove, reason };
}

void Trainer::AIplay() {
    EpisodeResult result = playEpisodeNoUpdate();

    for (const Board &b : currentBoards)
        Logger::board() << b;

    if (result.steps != 0)
        Logger::log() << agent.logDecision(currentBoards[currentBoards.size() - 1], result.lastMove,
                                           EpisodeResult::reasonToString(result.reason));

    Logger::RUN_DONE() << "AI"
                << " Steps " << result.steps
                << " Length " << result.length
                << " qtable " << agent.q_table.size()
                ;
}

void Trainer::AIplayBatch(unsigned runs) {
    EpochStats stats;
    std::vector<Board> bestBoards;
    Move bestBoardLastMove = Move::Up;
    EpisodeResult::EndReason bestBoardLastMoveReason = EpisodeResult::EndReason::MaxSteps;
    unsigned bestLength = 0;
    bool haveBest = false;

    unsigned deaths = 0, maxStepsHit = 0, timeouts = 0;

    for (unsigned i = 0; i < runs; ++i) {
        EpisodeResult result = playEpisodeNoUpdate();

        stats.add(result.length, result.steps);

        switch (result.reason) {
            case EpisodeResult::EndReason::Death:         deaths++;     break;
            case EpisodeResult::EndReason::MaxSteps:      maxStepsHit++; break;
            case EpisodeResult::EndReason::NoFoodTimeout: timeouts++;   break;
        }

        if (!haveBest || result.length > bestLength) {
            bestLength = result.length;
            bestBoards = currentBoards;
            bestBoardLastMove = result.lastMove;
            bestBoardLastMoveReason = result.reason;
            haveBest = true;
        }
    }

    for (const Board &b : bestBoards)
        Logger::board() << b;

    if (!bestBoards.empty()) {
        Logger::log() << agent.logDecision(bestBoards[bestBoards.size() - 1], bestBoardLastMove,
                                           EpisodeResult::reasonToString(bestBoardLastMoveReason));
    }

    Logger::RUN_DONE() << "AI batch: " << runs << " runs"
        << std::fixed << std::setprecision(2)
        << " | length " << stats.minLength << "to" << stats.maxLength
        << " mean:" << stats.meanLength << " stddev:" << stats.stddevLength()
        << " | steps:" << stats.minSteps << "to" << stats.maxSteps
        << " mean:" << stats.meanSteps << " stddev:" << stats.stddevSteps()
        << " | died:" << deaths << " max_steps:" << maxStepsHit << " timeout:" << timeouts
        << " | qtable " << agent.q_table.size()
        << " | Showing best run: length:" << bestLength << " steps: " << bestBoards.size();
}

Trainer::EpisodeResult Trainer::trainEpisode(bool log) {
    Board board = pipe.genBoard(agent.rng());

    currentBoards.clear();
    currentBoards.push_back(board);

    unsigned step = 0;

    const int maxStepsSinceEat = config.MAX_STEPS / 10;
    int stepsSinceEat = 0;
    Move lastMove = Move::Up;
    EpisodeResult::EndReason reason = EpisodeResult::EndReason::MaxSteps;

    for (; step < config.MAX_STEPS; step++) {
        Vision vision(board);
        Move action = agent.chooseAction(vision);
        lastMove = action;
        Board next_board = board.doMove(action, agent.rng());

        float _reward = reward(next_board.moveRes);

        if (next_board.moveRes == MoveRes::Death) {
            std::string stateKey;
            if (agent.rayState) stateKey = agent.rayKeysByMove(vision)[static_cast<size_t>(action)];
            else stateKey = agent.state(vision);
            agent.updateQtableOnDeath(vision, action, _reward);
            currentBoards.push_back(next_board);
            reason = EpisodeResult::EndReason::Death;
            break;
        } else {
            Vision next_vision(next_board);
            agent.updateQtable(vision, action, _reward, next_vision);
        }

        if (next_board.moveRes == MoveRes::Green || next_board.moveRes == MoveRes::Red)
            stepsSinceEat = 0;
        else
            stepsSinceEat++;

        board = next_board;
        currentBoards.push_back(board);

        if (stepsSinceEat >= maxStepsSinceEat) {
            reason = EpisodeResult::EndReason::NoFoodTimeout;
            break;
        }
    }

    if (log) {
        Logger::QTABLE_SIZE(agent.q_table.size());
        Logger::log() << "Episode "
                    << _current_ep << "/" << config.EPISODES
                    << " Steps:" << step
                    << " Length:" << board.snakeLength()
                    << " qtable:" << agent.q_table.size()
                    ;
    }

    return EpisodeResult{ step, board.snakeLength(), lastMove, reason };
}

void Trainer::train() {
    const bool singleTestRun = config.EPISODES == 0;
    unsigned toRun;

    if (singleTestRun) {
        toRun = 1;
    } else {
        if (_current_ep >= config.EPISODES) {
            Logger::RUN_DONE() << "Training complete " << _current_ep
                << "/" << config.EPISODES << " episodes trained.";
            return;
        }

        unsigned remaining = config.EPISODES - _current_ep;
        toRun = std::max(1u, std::min(config.SAMPLE_PER_REPLAY, remaining));
    }

    EpochStats stats;
    std::vector<Board> bestBoards;
    Move bestBoardLastMove = Move::Up;
    EpisodeResult::EndReason bestBoardlastMoveReason = EpisodeResult::EndReason::MaxSteps;
    unsigned bestLength = 0;
    bool haveBest = false;

    for (unsigned i = 0; i < toRun; ++i) {
        bool shouldLog = (i == toRun - 1);

        EpisodeResult result = trainEpisode(shouldLog);
        agent.decayEpsilon();
        if (!singleTestRun) _current_ep += 1;

        stats.add(result.length, result.steps);

        if (!haveBest || result.length > bestLength) {
            bestLength = result.length;
            bestBoards = currentBoards;
            bestBoardLastMove = result.lastMove;
            bestBoardlastMoveReason = result.reason;
            haveBest = true;
        }
    }

    for (const Board &b : bestBoards)
        Logger::board() << b;

    if (!bestBoards.empty()) {
        Logger::log() << agent.logDecision(bestBoards[bestBoards.size() - 1],
                                           bestBoardLastMove,
                                           EpisodeResult::reasonToString(bestBoardlastMoveReason));
    }

    if (singleTestRun) {
        Logger::RUN_DONE() << "Test run: "
            << std::fixed << std::setprecision(2)
            << "length: " << bestLength << " steps: " << bestBoards.size();
    } else {
        Logger::RUN_DONE() << "Epoch summary: "
            << std::fixed << std::setprecision(2)
            << "length: " << stats.minLength << "to" << stats.maxLength
            << " mean:" << stats.meanLength << " stddev:" << stats.stddevLength()
            << " | steps:" << stats.minSteps << "to" << stats.maxSteps
            << " mean:" << stats.meanSteps << " stddev:" << stats.stddevSteps()
            << " | Showing best run: length:" << bestLength << " steps: " << bestBoards.size();
    }
}

float Trainer::reward(MoveRes moveRes) {
    switch (moveRes) {
        case MoveRes::Advance: return config.reward_advance;
        case MoveRes::Green:   return config.reward_green;
        case MoveRes::Red:     return config.reward_red;
        case MoveRes::Death:   return config.reward_death;
    }
    return 0; 
}