#include "environment.hpp"
#include "interpreter.hpp"
#include "snapshot.hpp"

void vision();

int ret = 0;

int main() {
    std::cout << "Testing Interpreter\n";
    std::cout << "===================\n";

    vision();

    return ret;
}

void vision() {
    Pipe p(Board(10, 10), {
        Pipe::RandomSpawn{Board::Cell::Head},
        Pipe::RandomConnectedSpawn{Board::Cell::Head, Board::Cell::Snake, 2},
        Pipe::RandomSpawn{Board::Cell::Red},
        Pipe::RandomSpawn{Board::Cell::Green},
        Pipe::RandomSpawn{Board::Cell::Green},
    });

    for (int i = 0; i < 20; i++) {
        if (i < 4) {
        } else if (i < 8) {
            p._board = Board(6, 6);
        } else if (i < 12) {
            p._board = Board(4, 18);
        } else if (i < 16) {
            p._board = Board(7, 32);
        }
        auto board = p.genBoard(i);
        auto vision = Vision(board);

        std::ostringstream name;
        name << "vision_" << p._board.x_dim << "x" << p._board.y_dim
             << "_seed_" << i << ".snap";

        std::ostringstream out;
        out << "SEED " << i << "\n"
            << "BOARD:\n" << board << "\n"
            << "VISION\n" << vision << "\n";

        auto res = snapshot::test(name.str(), out.str());
        if (res == snapshot::Res::Pass) {
            std::cout << "[OK] " <<  name.str() << "\n";
        } else {
            ret = 1;
        }
    }
}
