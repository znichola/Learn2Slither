#include "environment.hpp"
#include "interpreter.hpp"
#include "snapshot.hpp"

void vision();

int main() {
    std::cout << "Testing Environment\n";
    std::cout << "===================\n";

    vision();

    return 0;
}

void extracted(int seed, Board board, Move move) {
    for (int i = 0; i < 4; i++) {

        std::ostringstream name;
        name << "move_" << seed << "_" << move << "_" << i << ".snap";

        std::ostringstream out;

        try {
            out << move << " move " << i << "\n";
            auto [b, r] = board.doMove(move);
            board = b;
            out << "Move result " << r << "\n"
                << "BOARD:\n"
                << board << "\n";
        } catch (const std::exception &e) {
            out << "EXCEPTION THROWN\n"
                << e.what() << "\n";
        } catch (...) {
           out  << "EXCEPTION THROWN\n"
                << "Unknown exception\n";
        }

        snapshot::test(name.str(), out.str());
        std::cout << "[OK] " << name.str() << "\n";
    }
}

void vision() {
  Pipe p(Board(10, 10), {
                            Pipe::RandomSpawn{Board::Cell::Head},
                            Pipe::RandomConnectedSpawn{Board::Cell::Head,
                                                       Board::Cell::Snake, 2},
                            Pipe::RandomSpawn{Board::Cell::Red},
                            Pipe::RandomSpawn{Board::Cell::Green},
                            Pipe::RandomSpawn{Board::Cell::Green},
                        });

  auto seed = 42;
  auto board = p.genBoard(seed);
  extracted(seed, board, Move::Left);
  extracted(seed, board, Move::Right);
  extracted(seed, board, Move::Up);
  extracted(seed, board, Move::Down);
}
