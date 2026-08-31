#include "environment/Grid.h"

#include <cassert>

namespace eco {

Grid::Grid(int width, int height)
    : width_(width), height_(height), cells_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {}

Cell& Grid::at(int x, int y) {
    assert(x >= 0 && x < width_ && y >= 0 && y < height_);
    return cells_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
}

const Cell& Grid::at(int x, int y) const {
    assert(x >= 0 && x < width_ && y >= 0 && y < height_);
    return cells_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
}

} // namespace eco
