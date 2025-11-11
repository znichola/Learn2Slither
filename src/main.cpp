#include "Board.hpp"
#include "iostream"

int main() {
    Board board = Board(10, 10);

    // auto [board2, seed] = board.randomSpawn(Board::Cell::Red, 42);

    // std::cout << "new Seed " << seed << "\n";
    // std::cout << board2;

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
        std::cout << "SEED " << i << "\n" << gen;
    }
}
