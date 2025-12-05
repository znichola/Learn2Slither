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

    Board start = p.genBoard(42);


    std::cout << "Start:\n" << start << "\n";

    Board next = start;
    for (int i = 0; i < 6; i++) {
        auto res = next.doMove(Move::Down);
        next = res.first;
        std::cout << "Move result: " << res.second << "\n";
        std::cout << "Next:\n" << next << "\n";
        if (res.second == MoveRes::Death) break;
    }
}
