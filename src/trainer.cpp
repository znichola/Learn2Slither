#include "trainer.hpp"
#include "environment.hpp"
#include "interpreter.hpp"
#include "agent.hpp"

#include "logger.hpp"

Trainer::Trainer(const Config &config)
    :
    config(config), 
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


void Trainer::train() {
    Logger::log() << "Starting training for " << config.EPISODES
                  << " episodes, max steps per episode: " << config.MAX_STEPS;

    const unsigned logInterval = config.EPISODES / 10;

    for (unsigned episode = 1; episode <= config.EPISODES; episode++) {
        Board board = pipe.genBoard(episode);

        unsigned episode_score = 0;
        bool done = false;

        if (episode % logInterval == 0) {
            Logger::log() << "Episode " << episode << " qtable_length "
                          << agent.q_table.size() << " initial board:\n";
            Logger::board() << board;
        }

        for (unsigned step = 0; step < config.MAX_STEPS; step++) {
            Vision vision(board);
            Move action = agent.chooseAction(vision);
            auto nextSeed = agent.rng();
            auto [next_board, moveRes] = board.doMove(action, nextSeed);

            if (moveRes == MoveRes::Death)
                done = true;
            float _reward = reward(moveRes);

            Vision next_vision(next_board);
            board = next_board;

            agent.updateQtable(vision, action, _reward, next_vision);
            agent.decayEpsilon();

            episode_score += _reward > 0 ? 1 : 0;

            if (episode % logInterval == 0) {
                Logger::board() << board;
            }

            if (done)
            {
                if (episode % logInterval == 0)
                    Logger::log() << "Episode " << episode
                                  << " score: " << episode_score;
                break;
            }
        }
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