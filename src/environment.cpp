#include <iostream>
#include <random>
#include <algorithm>
#include <sstream>
#include <cassert>

#include "environment.hpp"
#include "logger.hpp"

Board::Board(unsigned int x, unsigned int y) : x_dim(x+2), y_dim(y+2) {
    for (unsigned int i = 0; i < x_dim * y_dim; i++) {
        if (i < x_dim) { // top wall
            _grid.push_back(Cell::Wall);
        } else if (i % x_dim == 0) { // left wall
            _grid.push_back(Cell::Wall);
        } else if ((i+1) % x_dim == 0) { // right wall
            _grid.push_back(Cell::Wall);
        } else if (i > (y_dim-1) * x_dim) { // bottom wall
            _grid.push_back(Cell::Wall);
        } else {
            _grid.push_back(Cell::Empty);
        }
    }
}

Board::Board(const std::string &board) {
    std::stringstream ss(board);
    std::string tmp;
    auto match = [](auto c) {
        switch (c) {
            case (' '): return Cell::Empty;
            case ('W'): return Cell::Wall;
            case ('H'): return Cell::Head;
            case ('S'): return Cell::Snake;
            case ('G'): return Cell::Green;
            case ('R'): return Cell::Red;
        }
        assert(false && "invalid cell in board initilization");
    };
    y_dim = 0;
    while (std::getline(ss, tmp, '\n')) {
        if (tmp.length() <= 0)
            continue;
        x_dim = tmp.length();
        for (auto c : tmp) {
            _grid.push_back(match(c));
        }
        y_dim++;
    }
    auto it = find(_grid.begin(), _grid.end(), Cell::Head);
    assert(it != _grid.end() && "board init, must have head");

    unsigned head = std::distance(_grid.begin(), it);
    _snake.push_back(head);
    while (true) {
        unsigned mask[] = {head+1, head-1, head+x_dim, head-x_dim};
        bool found = false;
        for (auto m : mask) {
            if (m >= _grid.size()) continue;
            assert(m > 0 && "board init bounds underflow check");
            assert(m < _grid.size() && "board init bounds underflow check");
            auto f_it = find(_snake.begin(), _snake.end(), m);
            if (_grid[m] == Cell::Snake && f_it == _snake.end()) {
                head = m;
                _snake.insert(_snake.begin(), head);
                found = true;
                break;
            }
        }
        if (!found) break;
    }
}

Board::Op Board::randomSpawn(Cell t, unsigned seed) const {
    std::mt19937 rng(seed);

    std::vector<unsigned> candidates;
    unsigned i = 0;
    for (const auto c : _grid) {
        if (c == Cell::Empty) {
            candidates.push_back(i);
        }
        i++;
    }
    if (candidates.empty()) {
        return {*this, seed};
    }
    std::uniform_int_distribution<unsigned> dist(0, candidates.size() - 1);
    i = candidates[dist(rng)];
    Board newBoard = *this;
    newBoard._grid[i] = t;
    if (t == Cell::Head) {
        newBoard._snake.push_back(i);
    }
    return {newBoard, rng()};
}

Board::Op Board::randomConnectedSpawn(
        Board::Cell h,
        Board::Cell t,
        unsigned length,
        unsigned seed,
        bool asSnake
        ) const {
    std::mt19937 rng(seed);

    Board newBoard = *this;

    auto it = find(_grid.begin(), _grid.end(), h);
    if (it == _grid.end()) {
        throw std::runtime_error("randomConnectedSpawn: head cell not found");
    }
    unsigned head = std::distance(_grid.begin(), it);

    for (unsigned i = 0; i < length; i++) {
        std::vector<unsigned> candidates;
        unsigned mask[] = {head+1, head-1, head+x_dim, head-x_dim};
        // TODO out of bounds checks! need to thinkover this approach
        for (auto m : mask) {
             if (m < newBoard._grid.size() && newBoard._grid[m] == Cell::Empty) {
                 candidates.push_back(m);
             }
        }
        if (candidates.size() == 0) {
            throw std::runtime_error(
                    "randomConnectedSpawn: no valid candidates");
        }
        newBoard._grid[head] = t;
        std::uniform_int_distribution<unsigned> dist(0, candidates.size() - 1);
        auto index = candidates[dist(rng)];
        if (asSnake) {
            newBoard._snake.push_back(index);
        }
        newBoard._grid[index] = h;
        head = index;
    }

    return {newBoard, rng()};
}

void printSnake(std::vector<unsigned> s) {
    std::cout << "{";
    for (auto i : s) {
        std::cout << i << ", ";
    }
    std::cout << "}\n";
}

Board Board::doMove(Move m, unsigned seed) const {
    if (moveRes == MoveRes::Death) {
        Logger::error() << "Cannot move, snake is already dead!";
        return *this;
    }
    if (_snake.size() <= 0) throw std::runtime_error("Snake must have length");

    Board b = *this;

    unsigned head = b._snake.back();
    unsigned tail = b._snake.front();

    long newHead = 0;
    switch (m) {
        case Move::Up:    newHead = head - b.x_dim; break;
        case Move::Down:  newHead = head + b.x_dim; break;
        case Move::Left:  newHead = head - 1;       break;
        case Move::Right: newHead = head + 1;       break;
    }
    if (newHead < 0 || newHead >= static_cast<long>(b._grid.size()))
        throw std::runtime_error("NewHead out of range");

    switch (b._grid[newHead]) {
        case Cell::Wall:  b.moveRes = MoveRes::Death; break;
        case Cell::Snake: b.moveRes = MoveRes::Death; break;
        case Cell::Empty: b.moveRes = MoveRes::Advance; break;
        case Cell::Green: b.moveRes = MoveRes::Green; break;
        case Cell::Red:   b.moveRes = MoveRes::Red; break;
        case Cell::Head:  throw std::runtime_error("Cannot move onto head");
    }

    b._grid[head] = Cell::Snake;
    b._grid[newHead] = Cell::Head;
    b._snake.push_back(newHead);
    if (b.moveRes != MoveRes::Green) {
        b._snake.erase(b._snake.begin());
        b._grid[tail] = Cell::Empty;
    }
    if (b.moveRes == MoveRes::Red && b.snakeLength() != 0) {
        b._grid[b._snake.front()] = Cell::Empty;
        b._snake.erase(b._snake.begin());
    }
    if (b.snakeLength() == 0) b.moveRes = MoveRes::Death;

    if (b.moveRes == MoveRes::Death) {
        auto rb = *this;
        rb.moveRes = MoveRes::Death;
        return rb;
    } else if (b.moveRes == MoveRes::Red) {
        auto rb = b.randomSpawn(Cell::Red, seed);
        return rb.first;
    }
    else if (b.moveRes == MoveRes::Green) {
        auto rb = b.randomSpawn(Cell::Green, seed);
        return rb.first;
    }
    return b;
}

unsigned Board::snakeLength() const {
    return _snake.size();
}

Board Pipe::pipe(const Board &board, unsigned seed, std::vector<Op> ops) {
  Board b = board;
  unsigned s = seed;
  for (const Op &op : ops) {
    if (std::holds_alternative<RandomSpawn>(op)) {
      const auto &_op = std::get<RandomSpawn>(op);
      auto [newBoard, newSeed] = b.randomSpawn(_op.t, s);
      b = newBoard;
      s = newSeed;
    } else if (std::holds_alternative<RandomConnectedSpawn>(op)) {
      const auto &_op = std::get<RandomConnectedSpawn>(op);
      auto [newBoard, newSeed] =
          b.randomConnectedSpawn(_op.h, _op.t, _op.length, s, true);
      b = newBoard;
      s = newSeed;
    }
  }
  return b;
}

Pipe Pipe::standardPipeline() {
    return  Pipe(Board(10, 10), {
                      Pipe::RandomSpawn{Board::Cell::Head},
                      Pipe::RandomConnectedSpawn{
                      Board::Cell::Head, Board::Cell::Snake, 2},
                      Pipe::RandomSpawn{Board::Cell::Red},
                      Pipe::RandomSpawn{Board::Cell::Green},
                      Pipe::RandomSpawn{Board::Cell::Green},
                  });
}
