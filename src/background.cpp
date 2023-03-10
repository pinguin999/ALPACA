#include "background.hpp"

#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>
#include <limits>

Background::Background(std::shared_ptr<Game> game, const std::string &spine_file) : SpineObject(game, spine_file, "Background")
{
    spSkeleton_updateWorldTransform(skeleton->skeleton);
    spSkeletonBounds_update(bounds, skeleton->skeleton, true);

    corners = getCorners();
}

Background::~Background()
{
}

bool Background::step(bool force)
{
    skeleton->step();
    spSkeleton_updateWorldTransform(skeleton->skeleton);
    spSkeletonBounds_update(bounds, skeleton->skeleton, true);

    return stepClickableRegions(force) || deleted;
}

bool Background::stepClickableRegions(bool force)
{
    if (auto _game = game.lock())
    {
        if (!force && _game->getDialogManager()->isActive())
            return false;

        if (!force && _game->getInactivLayerBorder() > layer)
            return false;

        jngl::Vec2 mousePos = _game->pointer->getPosition();
        auto collision = spine::spSkeletonBounds_containsPointNotMatchingName(bounds, "walkable_area", (float)mousePos.x - (float)position.x, (float)mousePos.y - (float)position.y);
        // TODO Double Click on Regions
        if (_game->pointer->primaryPressed() && !_game->pointer->isPrimaryAlreadyHandled() && bool(collision))
        {
            collision_script = collision->super.super.name;
            jngl::debug("clicked interactable region ");
            jngl::debugLn(collision_script);
            _game->pointer->setPrimaryHandled();
            _game->runAction(collision_script, getptr());
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
        if(_game->enableDebugDraw)
        {
            jngl::setColor(255, 0, 0);
            for (size_t i = 0; i < corners.size(); i++)
            {
                if (hasPathTo(_game->player->getPosition(), corners.at(i)))
                    jngl::drawLine(_game->player->getPosition(), corners.at(i));
            }

            jngl::setColor(255, 255, 0);
            auto debugPath = getPathToTarget(_game->player->getPosition(), _game->pointer->getPosition());
            for (size_t i = 0; i < debugPath.size(); i++)
            {
                if (i == 0)
                {
                    // jngl::drawLine(_game->player->getPosition(), debugPath[i]);
                    continue;
                }
                jngl::drawLine(debugPath[i - 1], debugPath[i]);
            }
        }
    }
#endif

    jngl::popMatrix();
}

// Quelle: https://www.youtube.com/watch?v=c065KoXooSw
bool lineIntersection(jngl::Vec2 a, jngl::Vec2 b, jngl::Vec2 c, jngl::Vec2 d)
{
    jngl::Vec2 r = (b - a);
    jngl::Vec2 s = (d - c);

    double e = r.x * s.y - r.y * s.x;
    double u = ((c.x - a.x) * r.y - (c.y - a.y) * r.x) / e;
    double t = ((c.x - a.x) * s.y - (c.y - a.y) * s.x) / e;

    // Intersection Point a + t * r
    // Expand line with 0.01 to get right resault on corners
    return (-0.01 <= u && u <= 1.01 && -0.01 <= t && t <= 1.01);
}

Node::Node(jngl::Vec2 coordinates_, Node *parent_): coordinates(coordinates_)
{
    parent = parent_;
    G = H = 0;
}

int Node::getScore()
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
    std::vector<Node *> openSet, closedSet;

    openSet.push_back(new Node(start));

    while (!openSet.empty())
    {
        auto current_it = openSet.begin();
        current = *current_it;

        for (auto it = openSet.begin(); it != openSet.end(); it++)
        {
            auto node = *it;
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

        // auto directions = getDirections(current->coordinates);
        auto directions = corners;
        directions.push_back(target);
        for (size_t i = 0; i < directions.size(); ++i)
        {
            bool hastpath = !hasPathTo(current->coordinates, directions[i]);
            auto visited = findNodeOnList(closedSet, directions[i]);

            if (hastpath || visited)
            {
                continue;
            }

            int totalCost = current->G + heuristic(current->coordinates, directions[i]);

            Node *successor = findNodeOnList(openSet, directions[i]);
            if (successor == nullptr)
            {
                successor = new Node(directions[i], current);
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

void Background::releaseNodes(std::vector<Node *> &nodes_) const
{
    for (auto it = nodes_.begin(); it != nodes_.end();)
    {
        delete *it;
        it = nodes_.erase(it);
    }
}

Node *Background::findNodeOnList(const std::vector<Node *> &nodes_, jngl::Vec2 coordinates_) const
{
    for (auto node : nodes_)
    {
        if (node->coordinates == coordinates_)
        {
            return node;
        }
    }
    return nullptr;
}

int Background::heuristic(jngl::Vec2 start, jngl::Vec2 target) const
{
    return std::sqrt((start.x - target.x) * (start.x - target.x) + (start.y - target.y) * (start.y - target.y));
}

bool Background::hasPathTo(jngl::Vec2 start, jngl::Vec2 target) const
{
    int count = 0;
    for (size_t i = 0; i < corners.size() - 1; i++)
    {
        if (lineIntersection(start, target, corners.at(i), corners.at(i + 1)))
            count++;
    }

    bool corner = false;

    if (std::any_of(corners.cbegin(), corners.cend(), [&target](jngl::Vec2 corn)
                    { return corn == target; }))
    {
        corner = true;
    }

    // TODO Clean up. Can be moved into first loop.
    bool startcorner = false;
    if (std::any_of(corners.cbegin(), corners.cend(), [&start](jngl::Vec2 corn)
                    { return corn == start; }))
    {
        startcorner = true;
    }

    if (startcorner && count <= 2)
    {
        return true;
    }

    return (corner && count <= 2) || (!corner && count == 0);
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
        auto polygonName = bounds->boundingBoxes[iPoly]->super.super.name;
        if(polygonName == std::string("walkable_area"))
        {
            // bounds->polygons
            for (int i = 0; i < (bounds->polygons[iPoly])->count; i += 2)
            {
                result.push_back(jngl::Vec2((bounds->polygons[iPoly])->vertices[i + 0], (bounds->polygons[iPoly])->vertices[i + 1]));
            }
            // Add first to the back again.
            result.push_back(jngl::Vec2((bounds->polygons[iPoly])->vertices[0], (bounds->polygons[iPoly])->vertices[1]));
            break;
        }
    }

    return result;
}

bool Background::is_walkable(jngl::Vec2 position) const
{
    auto walkableResult = spine::spSkeletonBounds_containsPointMatchingName(bounds, "walkable_area", (float)position.x, (float)position.y);
    if(!walkableResult)
        return false;

    // if there is an interactable region and a walkable spot,
    // just interact, don't walk there
    auto interactableResult = spine::spSkeletonBounds_containsPointNotMatchingName(bounds, "walkable_area", (float)position.x, (float)position.y);
    return !interactableResult;
}
