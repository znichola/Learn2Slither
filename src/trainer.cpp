#include "trainer.hpp"
#include "environment.hpp"
#include "interpreter.hpp"
#include "agent.hpp"

#include "logger.hpp"

Trainer::Trainer(const Config &config)
    : config(config), 
    agent(Agent{
        .alpha = config.alpha,
        .gamma = config.gamma,
        .epsilon = config.epsilon,
        .epsilon_decay = config.epsilon_decay,
        .epsilon_min = config.epsilon_min,
        .state = State::get(config.stateFn),
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

    for (; step < config.MAX_STEPS; step++) {
        Vision vision(board);
        Move action = agent.chooseActionNoUpdate(vision);
        Board next_board = board.doMove(action, agent.rng());

        if (next_board.moveRes == MoveRes::Death)
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

void Trainer::trainEpisode(bool log) {
    Board board = pipe.genBoard(agent.rng());

    unsigned step = 0;

    if (log) Logger::board() << board;

    const int maxStepsSinceEat = config.MAX_STEPS / 10;
    int stepsSinceEat = 0;
    for (; step < config.MAX_STEPS; step++) {
        Vision vision(board);
        Move action = agent.chooseAction(vision);
        Board next_board = board.doMove(action, agent.rng());

        float _reward = reward(next_board.moveRes);

        if (next_board.moveRes == MoveRes::Death) {
            agent.updateQtableOnDeath(vision, action, _reward);
            break;
        } else {
            Vision next_vision(next_board);
            agent.updateQtable(vision, action, _reward, next_vision);
        }

        if (next_board.moveRes == MoveRes::Green || next_board.moveRes == MoveRes::Red)
            stepsSinceEat = 0;
        else
            stepsSinceEat++;
        if (stepsSinceEat >= maxStepsSinceEat)
            break;

        board = next_board;

        if (log) Logger::board() << board;

    }
    if (log) {
        Logger::QTABLE_SIZE(agent.q_table.size());
        Logger::RUN_DONE() << "Episode " 
                    << _current_ep << "/" << config.EPISODES
                    << " Steps " << step
                    << " Length " << board.snakeLength()
                    << " qtable " << agent.q_table.size()
                    ;
    }
}

void Trainer::train() {
    if (_current_ep >= config.EPISODES) {
        Logger::RUN_DONE() << "Training complete " << _current_ep
            << "/" << config.EPISODES << " episodes trained.";
        return;
    }

    unsigned remaining = config.EPISODES - _current_ep;
    unsigned toRun = std::min(config.SAMPLE_PER_REPLAY, remaining);

    for (unsigned i = 0; i < toRun; ++i) {

        bool shouldLog = (i == toRun - 1);
        trainEpisode(shouldLog);
        _current_ep += 1;
    }
}

float Trainer::reward(MoveRes moveRes) {
    // TODO: Maybe this can be switched up with more sofisticated reasoning
    switch (moveRes) {
        case MoveRes::Advance: return config.reward_advance;
        case MoveRes::Green:   return config.reward_green;
        case MoveRes::Red:     return config.reward_red;
        case MoveRes::Death:   return config.reward_death;
    }
    return 0; 
}