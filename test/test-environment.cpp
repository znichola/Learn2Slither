#include <ranges>

#include "environment.hpp"
#include "interpreter.hpp"
#include "snapshot.hpp"

void move();

int ret = 0;

int main() {
    std::cout << "Testing Environment\n";
    std::cout << "===================\n";

    move();

    return ret;
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

        auto res = snapshot::test(name.str(), out.str());
        if (res == snapshot::Res::Pass) {
            std::cout << "[OK] " << name.str() << "\n";
        } else {
            ret = 1;
        }
    }
}

void move() {
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
    auto moves = {
        Move::Left, Move::Left, Move::Left, Move::Left,
        Move::Up, Move::Up, Move::Up, Move::Up, Move::Up
        };
    auto expected_res = {
        MoveRes::Advance, MoveRes::Green, MoveRes::Advance, MoveRes::Advance,
        MoveRes::Advance, MoveRes::Advance, MoveRes::Advance, MoveRes::Advance,
        MoveRes::Red
    };

    auto it1 = moves.begin();
    auto it2 = expected_res.begin();
    for (; it1 != moves.end() && it2 != expected_res.end(); ++it1, ++it2) {
        auto m = *it1;
        auto e = *it2;
        auto [b, r] = board.doMove(m);
        board = b;
        //std::cout << "RES: " << r <<  "\n" << board << "\n";
        assert(r == e && "Move res does not match expected");
    }
    assert(board.snakeLength() == 3 && "Shorter after each red apple");
    std::stringstream out;
    out << "BOARD:\n" << board << "\n";
    auto name = "Custom_red_apple_path";
    auto res = snapshot::test(name, out.str());
    if (res == snapshot::Res::Pass) {
        std::cout << "[OK] " << name << "\n";
    } else {
        ret = 1;
    }
}

