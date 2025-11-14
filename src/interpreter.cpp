#include "interpreter.hpp"

Action::Move Action::move(const Vision &vision) const {
    (void)vision;
    return Action::Move::Up;
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
