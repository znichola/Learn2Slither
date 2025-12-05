#include <ranges>
#include <random>

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
    std::mt19937 rng(seed);
    for (int i = 0; i < 4; i++) {

        std::ostringstream name;
        name << "move_" << seed << "_" << move << "_" << i << ".snap";

        std::ostringstream out;

        try {
            out << move << " move " << i << "\n";
            auto [b, r] = board.doMove(move, rng());
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
        Move::Down, Move::Down, Move::Left, Move::Left,
        Move::Left, Move::Left, Move::Left, Move::Left, Move::Left
        };
    auto expected_res = {
        MoveRes::Advance, MoveRes::Advance, MoveRes::Advance, MoveRes::Advance,
        MoveRes::Advance, MoveRes::Red, MoveRes::Advance, MoveRes::Green,
        MoveRes::Advance
    };

    std::mt19937 rng(seed);

    auto it1 = moves.begin();
    auto it2 = expected_res.begin();
    for (; it1 != moves.end() && it2 != expected_res.end(); ++it1, ++it2) {
        auto m = *it1;
        auto e = *it2;
        auto [b, r] = board.doMove(m, rng());
        board = b;
        // std::cout << "RES: " << r <<  "\n" << board << "\n";
        ASSERT_EQ(r, e, "Move res does not match expected");
    }
    ASSERT_EQ(board.snakeLength(), 3U, "Shorter after each red apple");
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

