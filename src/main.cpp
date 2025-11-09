#include "iostream"
#include "Board.hpp"

int main() {
    Board board = Board(10, 10);

    auto [board2, seed] = board.randomSpawn(Board::Cell::Red, 42);

    std::cout << "new Seed " << seed << "\n";
    std::cout << board2;

    Pipe p;

    Board gen = p.pipe(board, 42, {
            Pipe::RandomSpawn{Board::Cell::Red},
            Pipe::RandomSpawn{Board::Cell::Green},
            Pipe::RandomSpawn{Board::Cell::Red},
            });
    std::cout << "PIPELINE\n" << gen;
}
