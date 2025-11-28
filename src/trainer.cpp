#include "trainer.hpp"
#include "environment.hpp"
#include "interpreter.hpp"
#include "agent.hpp"


void train() {
    const unsigned EPISODES = 9000;
    const unsigned MAX_STEPS = 2000;

    //float epsilon = 1.0f;
    //const float epsilon_decay = 0.9995f;
    //const float epsilon_min = 0.05f;

    //unsigned total_score = 0;
    //unsigned best_score = 0;

    Agent agent;

    Pipe p = Pipe::standardPipeline();

    for (unsigned episode = 1; episode <= EPISODES; episode++) {
        Board board = p.genBoard(episode);

        unsigned episode_score = 0;
        bool done = false;

        //std::cout << "\n=========== Starting ep: " << episode 
         //   << " qtable_length " << agent.q_table.size()<< "\n";
//        std::cout << board;

        for (unsigned step = 0; step < MAX_STEPS; step++) {
            Vision vision(board);
            Move action = agent.chooseAction(vision);
            auto [next_board, moveRes] = board.doMove(action);

            if (moveRes == MoveRes::Death) done = true;
            float rewar = reward(moveRes);

            Vision next_vision(next_board);
            board = next_board;

            agent.updateQtable(vision, action, rewar, next_vision);
/*
            std::cout << "MOVE: " << action << " RES: " << moveRes << "\n";
            std::cout << "SNAKE:";
            for (auto s : board._snake) {
                std::cout << " " << s << ":" << board._grid[s];
            }
            std::cout << "\n" << board;
*/
            episode_score += rewar > 0 ? 1 : 0;
            if (done) {
                if (board.snakeLength() > 4) {

        std::cout << "\n=========== Starting ep: " << episode 
            << " qtable_length " << agent.q_table.size()<< "\n";
                std::cout << "episode score: " << episode_score
                    << " length " << board.snakeLength() << "\n";
                }
                break;
            }
        }
    }
}
