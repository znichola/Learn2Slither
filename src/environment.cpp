#include <iostream>
#include <random>

#include "environment.hpp"

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

Board::Op Board::randomSpawn(Board::Cell t, unsigned seed) const {
    std::minstd_rand0 r (seed);

    std::vector<unsigned> candidates;
    unsigned i = 0;
    for (const auto c : _grid) {
        if (c == Cell::Empty) {
            candidates.push_back(i);
        }
        i++;
    }
    i = candidates[r() % candidates.size()];
    Board newBoard = *this;
    newBoard._grid[i] = t;
    return {newBoard, r()};
}

Board::Op Board::randomConnectedSpawn(
        Board::Cell h,
        Board::Cell t,
        unsigned length,
        unsigned seed
        ) const {
    std::minstd_rand0 r (seed);

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
        auto index = candidates[r() % candidates.size()];
        newBoard._grid[index] = h;
        head = index;
    }

    return {newBoard, r()};
}

Board pipe(auto ops) {
    for (const auto &op : ops) {
        std::cout << op << "\n";
    }
    return Board(10, 10);
}
