#include "iostream"

#include "environment.hpp"
#include "interpreter.hpp"

int main() {
    Board board = Board(10, 10);

    Pipe p(board, {
                      Pipe::RandomSpawn{Board::Cell::Head},
                      Pipe::RandomConnectedSpawn{
                        Board::Cell::Head, Board::Cell::Snake, 2},
                      Pipe::RandomSpawn{Board::Cell::Red},
                      Pipe::RandomSpawn{Board::Cell::Green},
                      Pipe::RandomSpawn{Board::Cell::Green},
                  });

    for (unsigned i = 0; i < 10; i++) {
        auto gen = p.genBoard(i);
        auto vision = Vision(gen);
        std::cout << "SEED " << i << "\n" << gen << "\nVISION\n" << vision;
    }
}
