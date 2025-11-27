#include "interpreter.hpp"

float reward(MoveRes moveRes) {
    // TODO: Maybe this can be switched up with more sofisticated reasoning
    switch (moveRes) {
        case MoveRes::Advance: return -0.2;
        case MoveRes::Green:   return 0.8;
        case MoveRes::Red:     return -0.5;
        case MoveRes::Death:   return -1.0;
    }
    return 0; 
}

void loop() {
    Board board = Board(10, 10);
    Pipe pipe(board, {
                      Pipe::RandomSpawn{Board::Cell::Head},
                      Pipe::RandomConnectedSpawn{
                        Board::Cell::Head, Board::Cell::Snake, 2},
                      Pipe::RandomSpawn{Board::Cell::Red},
                      Pipe::RandomSpawn{Board::Cell::Green},
                      Pipe::RandomSpawn{Board::Cell::Green},
                  });
}
