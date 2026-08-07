#include "trainer.hpp"
#include "environment.hpp"
#include "interpreter.hpp"
#include "agent.hpp"

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
        Logger::error() << "Refusing to change state function on existing agent";
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

void Trainer::AIplay() {
    Board board = pipe.genBoard(agent.rng());

    unsigned step = 0;

    Logger::board() << board;

    const int maxStepsSinceEat = config.MAX_STEPS / 10;
    int stepsSinceEat = 0;
    for (; step < config.MAX_STEPS; step++) {
        Vision vision(board);
        Move action = agent.chooseActionNoUpdate(vision);
        Board next_board = board.doMove(action, agent.rng());

        if (next_board.moveRes == MoveRes::Death)
            break;

        if (next_board.moveRes == MoveRes::Green || next_board.moveRes == MoveRes::Red)
            stepsSinceEat = 0;
        else
            stepsSinceEat++;
        if (stepsSinceEat >= maxStepsSinceEat)
            break;

        board = next_board;

        Logger::board() << board;
    }
    Logger::RUN_DONE() << "AI"
                << " Steps " << step
                << " Length " << board._snake.size()
                << " qtable " << agent.q_table.size()
                ;
}

Trainer::EpisodeResult Trainer::trainEpisode(bool log) {
    Board board = pipe.genBoard(agent.rng());

    currentBoards.clear();
    currentBoards.push_back(board);

    unsigned step = 0;

    const int maxStepsSinceEat = config.MAX_STEPS / 10;
    int stepsSinceEat = 0;
    for (; step < config.MAX_STEPS; step++) {
        Vision vision(board);
        Move action = agent.chooseAction(vision);
        Board next_board = board.doMove(action, agent.rng());

        float _reward = reward(next_board.moveRes);

        if (next_board.moveRes == MoveRes::Death) {
            if (log) Logger::log() << agent.logDecision(vision, action);
            agent.updateQtableOnDeath(vision, action, _reward);
            currentBoards.push_back(next_board);
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

        if (stepsSinceEat >= maxStepsSinceEat)
            break;
    }

    if (log) {
        Logger::QTABLE_SIZE(agent.q_table.size());
        Logger::log() << "Episode "
                    << _current_ep << "/" << config.EPISODES
                    << " Steps " << step
                    << " Length " << board.snakeLength()
                    << " qtable " << agent.q_table.size()
                    ;
    }

    return EpisodeResult{ step, board.snakeLength() };
}

void Trainer::train() {
    if (_current_ep >= config.EPISODES) {
        Logger::RUN_DONE() << "Training complete " << _current_ep
            << "/" << config.EPISODES << " episodes trained.";
        return;
    }

    unsigned remaining = config.EPISODES - _current_ep;
    unsigned toRun = std::min(config.SAMPLE_PER_REPLAY, remaining);

    EpochStats stats;
    bestBoards.clear();
    unsigned bestLength = 0;
    bool haveBest = false;

    for (unsigned i = 0; i < toRun; ++i) {
        bool shouldLog = (i == toRun - 1);

        EpisodeResult result = trainEpisode(shouldLog);
        agent.decayEpsilon();
        _current_ep += 1;

        stats.add(result.length, result.steps);

        if (!haveBest || result.length > bestLength) {
            bestLength = result.length;
            bestBoards = currentBoards; // copy: currentBoards gets overwritten next episode
            haveBest = true;
        }
    }

    if (toRun > 1) {
        for (const Board &b : bestBoards)
            Logger::board() << b;

        Logger::RUN_DONE() << "Epoch summary: "
            << std::fixed << std::setprecision(2)
            << "length: " << stats.minLength << " to " << stats.maxLength 
            << " mean:" << stats.meanLength << " stddev:" << stats.stddevLength()
            << " | steps:" << stats.minSteps << " to " << stats.maxSteps
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