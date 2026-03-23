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
        .q_table = {}
        }),
      pipe(Board(config.board_x, config.board_y), {
        Pipe::RandomSpawn{Board::Cell::Head},
        Pipe::RandomConnectedSpawn{Board::Cell::Head, Board::Cell::Snake, 2},
        Pipe::RandomSpawn{Board::Cell::Red},
        Pipe::RandomSpawn{Board::Cell::Green},
        Pipe::RandomSpawn{Board::Cell::Green},
        }) {}

void Trainer::AIplay() {
    Board board = pipe.genBoard(agent.rng());

    unsigned step = 0;

    Logger::board() << board;

    for (; step < config.MAX_STEPS; step++) {
        Vision vision(board);
        Move action = agent.chooseAction(vision);
        Board next_board = board.doMove(action, agent.rng());

        if (next_board.moveRes == MoveRes::Death)
            break;

        board = next_board;

        Logger::board() << board;
    }
    Logger::log() << "Game finished after "
                << " Steps " << step
                << " Length " << board._snake.size()
                << " qtable " << agent.q_table.size()
                ;
}

void Trainer::trainEpisode(bool log) {
    Board board = pipe.genBoard(agent.rng());

    unsigned step = 0;

    if (log) Logger::board() << board;

    for (; step < config.MAX_STEPS; step++) {
        Vision vision(board);
        Move action = agent.chooseAction(vision);
        Board next_board = board.doMove(action, agent.rng());

        float _reward = reward(next_board.moveRes);

        Vision next_vision(next_board);
        
        agent.updateQtable(vision, action, _reward, next_vision);
        agent.decayEpsilon();

        if (next_board.moveRes == MoveRes::Death)
            break;

        board = next_board;

        if (log) Logger::board() << vision;
    }
    if (log) {
        Logger::board() << Vision(board);
        Logger::batch_done() << "Episode " 
                    << _current_ep << "/" << config.EPISODES
                    << " Steps " << step
                    << " Length " << board._snake.size()
                    << " qtable " << agent.q_table.size()
                    ;
    }
}

void Trainer::train() {
    if (_current_ep >= config.EPISODES) {
        Logger::log() << "Training complete: " << _current_ep
            << "/" << config.EPISODES << " episodes trained.";
        return;
    }

    unsigned remaining = config.EPISODES - _current_ep;
    unsigned toRun = std::min(config.BATCH_SIZE, remaining);

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