#include "environment.hpp"
#include "interpreter.hpp"

Vision::Vision(const Board &board) {
    auto head_it = std::find(
            board._grid.begin(), board._grid.end(), Board::Cell::Head);
    auto rhead_it = std::find(
            board._grid.rbegin(), board._grid.rend(), Board::Cell::Head);
    auto c = std::count(
            board._grid.begin(), board._grid.end(), Board::Cell::Head);
    if (c != 1 || head_it == board._grid.end()
            || rhead_it == board._grid.rend()) {
        return ;
        throw std::runtime_error("Vision: board must contain one head");
    }

    auto east_it = find(head_it, board._grid.end(), Board::Cell::Wall);
    _east = std::vector<Board::Cell>(head_it+1, east_it+1);

    auto rwest_it = find(rhead_it, board._grid.rend(), Board::Cell::Wall);
    _west = std::vector<Board::Cell>(rhead_it+1, rwest_it+1);

    unsigned step = board.x_dim;
    for (auto i = head_it + step;
            i<board._grid.end(); std::advance(i, step)) {
        _south.push_back(*i);
    }

    for (auto i = rhead_it + step;
           i<board._grid.rend(); std::advance(i, step)) {
        _north.push_back(*i);
    }
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


State::State(const Vision &vision) {
    using Cell = Board::Cell;
    value = 0;
    int bitPos = 0;

    auto packCell = [&](Cell c) {
        value |= (static_cast<uint64_t>(c) << bitPos);
        bitPos += 3; // 6 cell types, so 3 bits to reprisent
    };

    auto packDirection = [&](const std::vector<Cell>& dir) {
        const int max_vision = 4; // if over 8 it's too big to fit in value!
        for (int i = 0; i < max_vision; i++) {
            packCell(i < (int)dir.size() ? dir[i] : Cell::Empty);
        }

        Board::Cell closest = Cell::Empty;
        for (int i = max_vision; i < (int)dir.size(); i++) {
            if (dir[i] != Cell::Empty) {
                closest = dir[i];
                break;
            }
        }
        packCell(closest);
    };

    packDirection(vision._north);
    packDirection(vision._east);
    packDirection(vision._south);
    packDirection(vision._west);
}

