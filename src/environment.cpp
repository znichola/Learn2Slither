#include <iostream>
#include <random>

#include "environment.hpp"

Board::Board(unsigned int x, unsigned int y) : x_dim(x+2), y_dim(y+2) {
    for (unsigned int i = 0; i < x_dim * y_dim; i++) {
        if (i < x_dim) { // top wall
            grid.push_back(Cell::Wall);
        } else if (i % x_dim == 0) { // left wall
            grid.push_back(Cell::Wall);
        } else if ((i+1) % x_dim == 0) { // right wall
            grid.push_back(Cell::Wall);
        } else if (i > (y_dim-1) * x_dim) { // bottom wall
            grid.push_back(Cell::Wall);
        } else {
            grid.push_back(Cell::Empty);
        }
    }
}

Board::Op Board::randomSpawn(Board::Cell t, unsigned seed) const {
    std::minstd_rand0 r (seed);

    std::vector<unsigned> candidates;
    unsigned i = 0;
    for (const auto c : grid) {
        if (c == Cell::Empty) {
            candidates.push_back(i);
        }
        i++;
    }
    i = candidates[r() % candidates.size()];
    Board newBoard = *this;
    newBoard.grid[i] = t;
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

    auto it = find(grid.begin(), grid.end(), h);
    if (it == grid.end()) {
        throw std::runtime_error("randomConnectedSpawn: head cell not found");
    }
    unsigned head = std::distance(grid.begin(), it);

    for (unsigned i = 0; i < length; i++) {
        std::vector<unsigned> candidates;
        unsigned mask[] = {head+1, head-1, head+x_dim, head-x_dim};
        // TODO out of bounds checks! need to thinkover this approach
        for (auto m : mask) {
             if (m < newBoard.grid.size() && newBoard.grid[m] == Cell::Empty) {
                 candidates.push_back(m);
             }
        }
        if (candidates.size() == 0) {
            throw std::runtime_error(
                    "randomConnectedSpawn: no valid candidates");
        }
        newBoard.grid[head] = t;
        auto index = candidates[r() % candidates.size()];
        newBoard.grid[index] = h;
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
