#include "background.hpp"

#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>
#include <limits>

Background::Background(const std::shared_ptr<Game> &game, const std::string &spine_file) : SpineObject(game, spine_file, "Background")
{
    step();
}

bool Background::step(bool force)
{
    skeleton->step();
    spSkeleton_updateWorldTransform(skeleton->skeleton);
    spSkeletonBounds_update(bounds, skeleton->skeleton, 1);
    corners = getCorners();
    forbidden_corners = getForbiddenCorners();
    // corners.insert( corners.end(), forbidden_corners.begin(), forbidden_corners.end() );

    return stepClickableRegions(force) || deleted;
}

bool Background::stepClickableRegions(bool force)
{
    if (auto _game = game.lock())
    {
        if (!force && _game->getDialogManager()->isActive())
        {
            return false;
        }

        if (!force && _game->getInactivLayerBorder() > layer)
        {
            return false;
        }

        if (!force && _game->player && !_game->player->interruptible)
        {
            return false;
        }

        if (_game->pointer && _game->pointer->primaryPressed() && visible && !_game->pointer->isPrimaryAlreadyHandled())
        {
            const jngl::Vec2 mousePos = _game->pointer->getPosition();
            auto *collision = spine::spSkeletonBounds_containsPointNotMatchingName(bounds, "walkable_area", static_cast<float>(mousePos.x) - static_cast<float>(position.x), static_cast<float>(mousePos.y) - static_cast<float>(position.y));
            // TODO Double Click on Regions
            if (collision)
            {
                collision_script = collision->super.super.name;
                jngl::debug("clicked interactable region ");
                jngl::debugLn(collision_script);
                _game->pointer->setPrimaryHandled();
                _game->runAction(collision_script, getptr());
            }
        }
    }

    return deleted;
}

void Background::draw() const
{
    jngl::pushMatrix();
    jngl::translate(position);
    jngl::rotate(getRotation());

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        skeleton->debugdraw = _game->enableDebugDraw;
    }
#endif

    skeleton->draw();

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->enableDebugDraw)
        {
            jngl::setColor(255, 0, 0);
            if (_game->player)
            {
                for (auto corner : corners)
                {
                    if (hasPathTo(_game->player->getPosition(), corner))
                    {
                        jngl::drawLine(_game->player->getPosition(), corner);
                    }
                }

                jngl::setColor(0, 0, 0);
                for (auto forbidden_area : forbidden_corners)
                {
                    for (size_t i = 1; i < forbidden_area.size(); i++)
                    {
                        jngl::drawLine(forbidden_area.at(i), forbidden_area.at(i - 1));
                    }
                }

                jngl::setColor(0, 0, 255);
                for (auto forbidden_area : forbidden_corners)
                {
                    for (auto forbidden_corner : forbidden_area)
                    {
                        if (hasPathTo(_game->player->getPosition(), forbidden_corner))
                        {
                            jngl::drawLine(_game->player->getPosition(), forbidden_corner);
                        }
                    }
                }

                jngl::setColor(255, 255, 0);
                auto debugPath = getPathToTarget(_game->player->getPosition(), _game->pointer->getPosition());
                for (size_t i = 1; i < debugPath.size(); i++)
                {
                    jngl::drawLine(debugPath[i - 1], debugPath[i]);
                }
            }

            // for (auto corner : corners)
            // {
            //     for (auto forcorner : forbidden_corners)
            //     {
            //         if (hasPathTo(forcorner, corner))
            //         {
            //             jngl::drawLine(forcorner, corner);
            //         }
            //     }
            // }

            // jngl::setColor(255, 0, 0);
            // for (auto corner : forbidden_corners)
            // {
            //     for (auto forcorner : forbidden_corners)
            //     {
            //         if (hasPathTo(forcorner, corner))
            //         {
            //             jngl::drawLine(forcorner, corner);
            //         }
            //     }
            // }

            auto point_names = getPointNames();
            for (const auto &point_name : point_names)
            {
                auto pos = getPoint(point_name);
                if (pos)
                {
                    jngl::drawPoint(pos->x, pos->y);
                    jngl::Text bbname;
                    bbname.setText(point_name);
                    bbname.setAlign(jngl::Alignment::CENTER);
                    bbname.setCenter(pos->x, pos->y);
                    bbname.draw();
                }
            }
        }
    }
#endif

    jngl::popMatrix();
}

// Quelle: https://www.youtube.com/watch?v=c065KoXooSw
bool lineIntersection(jngl::Vec2 a, jngl::Vec2 b, jngl::Vec2 c, jngl::Vec2 d)
{
    jngl::Vec2 const r = (b - a);
    jngl::Vec2 const s = (d - c);

    double const e = r.x * s.y - r.y * s.x;
    double const u = ((c.x - a.x) * r.y - (c.y - a.y) * r.x) / e;
    double const t = ((c.x - a.x) * s.y - (c.y - a.y) * s.x) / e;

    // Intersection Point a + t * r
    // Expand line with 0.01 to get right resault on corners
    return (-0.01 <= u && u <= 1.01 && -0.01 <= t && t <= 1.01);
}

Node::Node(jngl::Vec2 coordinates_, Node *parent_) : coordinates(coordinates_), parent(parent_)
{

    G = H = 0;
}

int Node::getScore() const
{
    return G + H;
}

std::deque<jngl::Vec2> Background::getPathToTarget(jngl::Vec2 start, jngl::Vec2 target) const
{
    // Debug Positions
    //     start = {
    // -595.38133024921717,
    // 264.28874734607211};
    //     target = {
    // 550.99726190217416,
    // 511.59235668789813
    //         };

    std::deque<jngl::Vec2> path;

    if (!is_walkable(target))
    {
        return path;
    }

    Node *current = nullptr;
    std::vector<Node *> openSet;
    std::vector<Node *> closedSet;

    openSet.push_back(new Node(start));

    while (!openSet.empty())
    {
        auto current_it = openSet.begin();
        current = *current_it;

        for (auto it = openSet.begin(); it != openSet.end(); it++)
        {
            auto *const node = *it;
            if (node->getScore() <= current->getScore())
            {
                current = node;
                current_it = it;
            }
        }

        if (current->coordinates == target)
        {
            break;
        }

        closedSet.push_back(current);
        openSet.erase(current_it);

        auto directions = corners;
        for (auto forbidden_area : forbidden_corners)
        {
            directions.insert(directions.end(), forbidden_area.begin(), forbidden_area.end());
        }
        directions.push_back(target);
        for (auto direction : directions)
        {
            const bool hastpath = !hasPathTo(current->coordinates, direction);
            auto *const visited = findNodeOnList(closedSet, direction);

            if (hastpath || visited)
            {
                continue;
            }

            const int totalCost = current->G + heuristic(current->coordinates, direction);

            Node *successor = findNodeOnList(openSet, direction);
            if (successor == nullptr)
            {
                successor = new Node(direction, current);
                successor->G = totalCost;
                successor->H = heuristic(successor->coordinates, target);
                openSet.push_back(successor);
            }
            else if (totalCost < successor->G)
            {
                successor->parent = current;
                successor->G = totalCost;
            }
        }
    }

    while (current != nullptr)
    {
        path.emplace_front(current->coordinates);
        current = current->parent;
    }

    releaseNodes(openSet);
    releaseNodes(closedSet);

    return path;
}

void Background::releaseNodes(std::vector<Node *> &nodes_)
{
    for (auto it = nodes_.begin(); it != nodes_.end();)
    {
        delete *it;
        it = nodes_.erase(it);
    }
}

Node *Background::findNodeOnList(const std::vector<Node *> &nodes_, jngl::Vec2 coordinates_)
{
    for (auto *node : nodes_)
    {
        if (node->coordinates == coordinates_)
        {
            return node;
        }
    }
    return nullptr;
}

int Background::heuristic(jngl::Vec2 start, jngl::Vec2 target)
{
    return std::sqrt((start.x - target.x) * (start.x - target.x) + (start.y - target.y) * (start.y - target.y));
}

bool Background::hasPathTo(jngl::Vec2 start, jngl::Vec2 target) const
{
    int count = 0;

    if (corners.empty())
    {
        return false;
    }

    for (size_t i = 0; i < corners.size() - 1; i++)
    {
        if (lineIntersection(start, target, corners.at(i), corners.at(i + 1)))
        {
            count++;
        }
    }

    bool corner = false;

    if (std::any_of(corners.cbegin(), corners.cend(), [&target](jngl::Vec2 corn)
                    { return corn == target; }))
    {
        corner = true;
    }

    std::vector<bool> is_forbidden_corner;
    std::vector<int> forbidden_counts;
    for (auto forbidden_area : forbidden_corners)
    {
        bool forbidden_corner = false;
        if (std::any_of(forbidden_area.cbegin(), forbidden_area.cend(), [&target](jngl::Vec2 corn)
                        { return corn == target; }))
        {
            forbidden_corner = true;
        }
        is_forbidden_corner.push_back(forbidden_corner);

        int forbidden_count = 0;
        for (size_t i = 0; i < forbidden_area.size() - 1; i++)
        {
            if (lineIntersection(start, target, forbidden_area.at(i), forbidden_area.at(i + 1)))
            {
                forbidden_count++;
            }
        }
        forbidden_counts.push_back(forbidden_count);
    }

    int i = 0;
    for (auto forbidden_area : forbidden_corners)
    {
        if ((is_forbidden_corner[i] && (forbidden_counts[i] > 2 || count != 0)) ||
            (corner && forbidden_counts[i] > 0))
        {
            return false;
        }
        i++;
    }

    // TODO Clean up. Can be moved into first loop.
    bool startcorner = false;
    if (std::any_of(corners.cbegin(), corners.cend(), [&start](jngl::Vec2 corn)
                    { return corn == start; }))
    {
        startcorner = true;
    }

    i = 0;
    bool forbidden_startcorner = false;
    for (auto forbidden_area : forbidden_corners)
    {
        if (std::any_of(forbidden_area.cbegin(), forbidden_area.cend(), [&start](jngl::Vec2 corn)
                        { return corn == start; }))
        {
            forbidden_startcorner = true;
        }

        if ((startcorner && !forbidden_startcorner && count <= 2 && forbidden_counts[i] == 0) || (forbidden_startcorner && !startcorner && forbidden_counts[i] <= 2 && count == 0))
        {
            return true;
        }
        i++;
    }

    bool bla = true;
    i = 0;
    for (auto forbidden_corner : is_forbidden_corner)
    {

        bla = bla && ((corner && forbidden_counts[i] <= 2 && count <= 2) ||
                      (!corner && forbidden_counts[i] == 0 && count == 0) ||
                      (forbidden_corner && forbidden_counts[i] <= 2 && count <= 2) ||
                      (!forbidden_corner && forbidden_counts[i] == 0 && count == 0));
        i++;
    }

    return bla;
}

std::vector<jngl::Vec2> Background::getCorners() const
{
    std::vector<jngl::Vec2> result;
    if (bounds->count == 0)
    {
        return result;
    }
    for (int iPoly = 0; iPoly < bounds->count; iPoly++)
    {
        const auto *polygonName = bounds->boundingBoxes[iPoly]->super.super.name;
        if (polygonName == std::string("walkable_area"))
        {
            // bounds->polygons
            for (int i = 0; i < (bounds->polygons[iPoly])->count; i += 2)
            {
                result.emplace_back((bounds->polygons[iPoly])->vertices[i + 0], (bounds->polygons[iPoly])->vertices[i + 1]);
            }
            // Add first to the back again.
            result.emplace_back((bounds->polygons[iPoly])->vertices[0], (bounds->polygons[iPoly])->vertices[1]);
            break;
        }
    }

    return result;
}

std::vector<std::vector<jngl::Vec2>> Background::getForbiddenCorners() const
{
    std::vector<std::vector<jngl::Vec2>> result;

    std::vector<jngl::Vec2> forbidden_area;

    if (auto _game = game.lock())
    {
        for (const auto &obj : _game->gameObjects)
        {
            for (int iPoly = 0; iPoly < obj->bounds->count; iPoly++)
            {
                const auto *polygonName = obj->bounds->boundingBoxes[iPoly]->super.super.name;
                if (polygonName == std::string("non_walkable_area"))
                {
                    // obj->bounds->polygons
                    for (int i = 0; i < (obj->bounds->polygons[iPoly])->count; i += 2)
                    {
                        forbidden_area.emplace_back(((obj->bounds->polygons[iPoly])->vertices[i + 0]) + obj->getPosition().x, ((obj->bounds->polygons[iPoly])->vertices[i + 1]) + obj->getPosition().y);
                    }
                    // Add first to the back again.
                    forbidden_area.emplace_back(((obj->bounds->polygons[iPoly])->vertices[0]) + obj->getPosition().x, ((obj->bounds->polygons[iPoly])->vertices[1]) + obj->getPosition().y);
                    result.emplace_back(forbidden_area);
                    forbidden_area.clear();
                }
            }
        }
    }
    return result;
}

bool Background::is_walkable(jngl::Vec2 position) const
{
    auto *walkableResult = spine::spSkeletonBounds_containsPointMatchingName(bounds, "walkable_area", static_cast<float>(position.x), static_cast<float>(position.y));
    if (!walkableResult)
    {
        return false;
    }

    if (auto _game = game.lock())
    {
        for (const auto &obj : _game->gameObjects)
        {
            auto *non_walkable = spine::spSkeletonBounds_containsPointMatchingName(obj->bounds, "non_walkable_area", static_cast<float>(position.x) - static_cast<float>(obj->getPosition().x), static_cast<float>(position.y) - static_cast<float>(obj->getPosition().y));
            if (non_walkable)
            {
                jngl::debugLn("non walkable");
                return false;
            }
        }
    }

    // if there is an interactable region and a walkable spot,
    // just interact, don't walk there
    auto *interactableResult = spine::spSkeletonBounds_containsPointNotMatchingName(bounds, "walkable_area", static_cast<float>(position.x), static_cast<float>(position.y));
    return interactableResult == nullptr;
}
