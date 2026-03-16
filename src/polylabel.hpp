#pragma once
#include "jngl/Vec2.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>

// Source https://github.com/mapbox/polylabel
// Docs https://blog.mapbox.com/a-new-algorithm-for-finding-a-visual-center-of-a-polygon-7c77e6492fbc

namespace mapbox {

namespace detail {

// get squared distance from a point to a segment
double getSegDistSq(const jngl::Vec2& p,
               const jngl::Vec2& a,
               const jngl::Vec2& b) {
    auto x = a.x;
    auto y = a.y;
    auto dx = b.x - x;
    auto dy = b.y - y;

    if (dx != 0 || dy != 0) {

        auto t = ((p.x - x) * dx + (p.y - y) * dy) / (dx * dx + dy * dy);

        if (t > 1) {
            x = b.x;
            y = b.y;

        } else if (t > 0) {
            x += dx * t;
            y += dy * t;
        }
    }

    dx = p.x - x;
    dy = p.y - y;

    return dx * dx + dy * dy;
}

// signed distance from point to polygon outline (negative if point is outside)
double pointToPolygonDist(const jngl::Vec2& point, const std::vector<std::vector<jngl::Vec2>>& polygon) {
    bool inside = false;
    auto minDistSq = std::numeric_limits<double>::infinity();

    for (const auto& ring : polygon) {
        for (std::size_t i = 0, len = ring.size(), j = len - 1; i < len; j = i++) {
            const jngl::Vec2& a = ring[i];
            const jngl::Vec2& b = ring[j];

            if ((a.y > point.y) != (b.y > point.y) &&
                (point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x)) inside = !inside;

            minDistSq = std::min(minDistSq, getSegDistSq(point, a, b));
        }
    }

    return (inside ? 1 : -1) * std::sqrt(minDistSq);
}

struct Cell {
    Cell(const jngl::Vec2& c_, double h_, const std::vector<std::vector<jngl::Vec2>>& polygon)
        : c(c_),
          h(h_),
          d(pointToPolygonDist(c, polygon)),
          max(d + h * std::sqrt(2))
        {}

    jngl::Vec2 c; // cell center
    double h; // half the cell size
    double d; // distance from cell center to polygon
    double max; // max distance to polygon within a cell
};

// get polygon centroid
Cell getCentroidCell(const std::vector<std::vector<jngl::Vec2>>& polygon) {
    double area = 0;
    jngl::Vec2 c { 0, 0 };
    const auto& ring = polygon.at(0);

    for (std::size_t i = 0, len = ring.size(), j = len - 1; i < len; j = i++) {
        const jngl::Vec2& a = ring[i];
        const jngl::Vec2& b = ring[j];
        auto f = a.x * b.y - b.x * a.y;
        c.x += (a.x + b.x) * f;
        c.y += (a.y + b.y) * f;
        area += f * 3;
    }

    return Cell(area == 0 ? ring.at(0) : c / area, 0, polygon);
}

} // namespace detail

std::tuple<jngl::Vec2, jngl::Vec2> create_envelope(std::vector<jngl::Vec2> const& geometry)
{
    double min_t = std::numeric_limits<double>::min();
    double max_t = std::numeric_limits<double>::max();

    jngl::Vec2 min(max_t, max_t);
    jngl::Vec2 max(min_t, min_t);

    for (auto point : geometry)
    {
        if (min.x > point.x) min.x = point.x;
        if (min.y > point.y) min.y = point.y;
        if (max.x < point.x) max.x = point.x;
        if (max.y < point.y) max.y = point.y;
    }

    return {min, max};
}

jngl::Vec2 polylabel(const std::vector<std::vector<jngl::Vec2>>& polygon, double precision = 1, bool debug = false) {
    using namespace detail;

    // find the bounding box of the outer ring
    auto envelope = create_envelope(polygon.at(0));

    jngl::Vec2 envelope_min = std::get<0>(envelope);
    jngl::Vec2 envelope_max = std::get<1>(envelope);

    const jngl::Vec2 size {
        envelope_max.x - envelope_min.x,
        envelope_max.y - envelope_min.y
    };

    const double cellSize = std::min(size.x, size.y);
    double h = cellSize / 2;

    // a priority queue of cells in order of their "potential" (max distance to polygon)
    auto compareMax = [] (const Cell& a, const Cell& b) {
        return a.max < b.max;
    };
    using Queue = std::priority_queue<Cell, std::vector<Cell>, decltype(compareMax)>;
    Queue cellQueue(compareMax);

    if (cellSize == 0) {
        return envelope_min;
    }

    // cover polygon with initial cells
    for (double x = envelope_min.x; x < envelope_max.x; x += cellSize) {
        for (double y = envelope_min.y; y < envelope_max.y; y += cellSize) {
            cellQueue.push(Cell({x + h, y + h}, h, polygon));
        }
    }

    // take centroid as the first best guess
    auto bestCell = getCentroidCell(polygon);

    // second guess: bounding box centroid
    Cell bboxCell(envelope_min + size / 2.0, 0, polygon);
    if (bboxCell.d > bestCell.d) {
        bestCell = bboxCell;
    }

    auto numProbes = cellQueue.size();
    while (!cellQueue.empty()) {
        // pick the most promising cell from the queue
        auto cell = cellQueue.top();
        cellQueue.pop();

        // update the best cell if we found a better one
        if (cell.d > bestCell.d) {
            bestCell = cell;
            if (debug) std::cout << "found best " << ::round(1e4 * cell.d) / 1e4 << " after " << numProbes << " probes" << std::endl;
        }

        // do not drill down further if there's no chance of a better solution
        if (cell.max - bestCell.d <= precision) continue;

        // split the cell into four cells
        h = cell.h / 2;
        cellQueue.push(Cell({cell.c.x - h, cell.c.y - h}, h, polygon));
        cellQueue.push(Cell({cell.c.x + h, cell.c.y - h}, h, polygon));
        cellQueue.push(Cell({cell.c.x - h, cell.c.y + h}, h, polygon));
        cellQueue.push(Cell({cell.c.x + h, cell.c.y + h}, h, polygon));
        numProbes += 4;
    }

    if (debug) {
        std::cout << "num probes: " << numProbes << std::endl;
        std::cout << "best distance: " << bestCell.d << std::endl;
    }

    return bestCell.c;
}

} // namespace mapbox
