#include "background.hpp"

#include "jngl/matrix.hpp"
#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>

Background::Background(const std::shared_ptr<Game> &game, const std::string &spine_file) : SpineObject(game, spine_file, "Background")
{
    stepSpineAndNavigation();
}

void Background::stepSpineAndNavigation()
{
    skeleton->step();
    skeleton->skeleton->update(1.0/60.0);
    skeleton->skeleton->updateWorldTransform(spine::Physics_Update);
    bounds->update(*skeleton->skeleton, true);
    corners = getCorners();
    forbidden_corners = getForbiddenCorners();
}

bool Background::step(bool force)
{
    stepSpineAndNavigation();
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
			const jngl::Vec2 mousePos = _game->pointer->getWorldPosition();
			auto* collision = spSkeletonBounds_containsPointNotMatchingName(
			    bounds.get(), "walkable_area",
			    static_cast<float>(mousePos.x) - static_cast<float>(position.x),
			    static_cast<float>(mousePos.y) - static_cast<float>(position.y));
			// TODO Double Click on Regions
			if (collision) {
				collision_script = collision->getName().buffer();
				jngl::debug("clicked interactable region {}", collision_script);
				_game->pointer->setPrimaryHandled();
				_game->runAction(collision_script, getptr());
			}
		}
	}

	return deleted;
}

void Background::draw() const
{
#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        skeleton->debugdraw = _game->enableDebugDraw;
    }
#endif

    auto mv = jngl::modelview().translate(position).rotate(getRotation());
    skeleton->draw(mv);

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->enablezMapDebugDraw && sprite)
        {
            sprite->draw(mv);
        }
    }
#endif

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
                        jngl::drawLine(mv, _game->player->getPosition(), corner);
                    }
                }

                jngl::setColor(0, 0, 0);
                for (const auto &forbidden_area : forbidden_corners)
                {
                    for (size_t i = 1; i < forbidden_area.size(); i++)
                    {
                        jngl::drawLine(mv, forbidden_area.at(i), forbidden_area.at(i - 1));
                    }
                }

                jngl::setColor(0, 0, 255);
                for (const auto &forbidden_area : forbidden_corners)
                {
                    for (auto forbidden_corner : forbidden_area)
                    {
                        if (hasPathTo(_game->player->getPosition(), forbidden_corner))
                        {
                            jngl::drawLine(mv, _game->player->getPosition(), forbidden_corner);
                        }
                    }
                }

                jngl::setColor(255, 255, 0);
                auto debugPath = getPathToTarget(_game->player->getPosition(), _game->pointer->getWorldPosition());
                for (size_t i = 1; i < debugPath.size(); i++)
                {
                    jngl::drawLine(mv, debugPath[i - 1], debugPath[i]);
                }
            }

            auto point_names = getPointNames();
            for (const auto &point_name : point_names)
            {
                auto pos = getPoint(point_name);
                if (pos)
                {
                    jngl::drawCircle(jngl::Mat3(mv).translate(jngl::Vec2(pos->x, pos->y)), 1);
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
}

enum class Result {
    INTERSECTION,
    NO_INTERSECTION,
    TWO_POINTS_EQUAL,
};

// Quelle: https://www.youtube.com/watch?v=c065KoXooSw
static Result lineIntersection(jngl::Vec2 a, jngl::Vec2 b, jngl::Vec2 c, jngl::Vec2 d)
{
    if (boost::qvm::mag_sqr(a - c) < 0.1) {
        return boost::qvm::mag_sqr(b - d) < 0.1 ? Result::TWO_POINTS_EQUAL
                                                : Result::NO_INTERSECTION;
    }
    if (boost::qvm::mag_sqr(a - d) < 0.1) {
        return boost::qvm::mag_sqr(b - c) < 0.1 ? Result::TWO_POINTS_EQUAL
                                                : Result::NO_INTERSECTION;
    }
    if (boost::qvm::mag_sqr(b - d) < 0.1 || boost::qvm::mag_sqr(b - c) < 0.1) {
        return Result::NO_INTERSECTION;
    }
    jngl::Vec2 const r = (b - a);
    jngl::Vec2 const s = (d - c);

    double const e = r.x * s.y - r.y * s.x;
    double const u = ((c.x - a.x) * r.y - (c.y - a.y) * r.x) / e;
    double const t = ((c.x - a.x) * s.y - (c.y - a.y) * s.x) / e;

    // Intersection Point a + t * r
    return (0 <= u && u <= 1 && 0 <= t && t <= 1) ? Result::INTERSECTION
                                                  : Result::NO_INTERSECTION;
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

        if (boost::qvm::mag_sqr(current->coordinates - target) < 1)
        {
            break;
        }

        closedSet.push_back(current);
        openSet.erase(current_it);

        auto directions = corners;
        for (const auto &forbidden_area : forbidden_corners)
        {
            directions.insert(directions.end(), forbidden_area.begin(), forbidden_area.end());
        }
        directions.push_back(target);
        for (auto direction : directions)
        {
            const auto visited = findNodeOnList(closedSet, direction);
            if (visited || !hasPathTo(current->coordinates, direction)) {
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
    for (const auto node : nodes_)
    {
        if (boost::qvm::mag_sqr(node->coordinates - coordinates_) < 0.5)
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
    if (corners.empty())
    {
        return false;
    }
    if (boost::qvm::mag_sqr(target - start) < 0.1) {
        return false;
    }

    bool twoPointsEqual = false;
    for (size_t i = 0; i < corners.size() - 1; i++)
    {
        // do we intersect with an edge of the walkable area?
        switch (lineIntersection(start, target, corners.at(i), corners.at(i + 1)))
        {
        case Result::INTERSECTION:
            return false;
        case Result::NO_INTERSECTION:
            break;
        case Result::TWO_POINTS_EQUAL:
            twoPointsEqual = true;
            break;
        }
    }

    if (!forbidden_corners.empty())
    {
        for (auto forbidden_corner : forbidden_corners){
            for (size_t i = 0; i < forbidden_corner.size() - 1; i++)
            {
                switch (lineIntersection(start, target, forbidden_corner.at(i), forbidden_corner.at(i + 1)))
                {
                using enum Result;
                case Result::INTERSECTION:
                    return false;
                case Result::NO_INTERSECTION:
                    break;
                case Result::TWO_POINTS_EQUAL:
                    twoPointsEqual = true;
                    break;
                }
            }
        }
    }

    if (twoPointsEqual) {
        return true;
    }
    auto direction = boost::qvm::normalized(target - start);
    return is_walkable(start + direction); // move 1 pixel and see if that wouldn't leave the walkable area
}

std::vector<jngl::Vec2> Background::getCorners() const
{
    std::vector<jngl::Vec2> result;
    spine::Vector<spine::BoundingBoxAttachment*>& boundingBoxes = bounds->getBoundingBoxes();
    spine::Vector<spine::Polygon*>& polygons = bounds->getPolygons();
    if (boundingBoxes.size() == 0)
    {
        return result;
    }
    for (size_t iPoly = 0; iPoly < boundingBoxes.size(); iPoly++)
    {
        const auto *polygonName = boundingBoxes[iPoly]->getName().buffer();
        if (polygonName == std::string("walkable_area"))
        {
            // bounds->polygons
            for (size_t i = 0; i < polygons[iPoly]->_vertices.size(); i += 2)
            {
                result.emplace_back(polygons[iPoly]->_vertices[i + 0], polygons[iPoly]->_vertices[i + 1]);
            }
            // Add first to the back again.
            result.emplace_back(polygons[iPoly]->_vertices[0], polygons[iPoly]->_vertices[1]);
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
            spine::Vector<spine::BoundingBoxAttachment*>& boundingBoxes = obj->bounds->getBoundingBoxes();
            spine::Vector<spine::Polygon*>& polygons = obj->bounds->getPolygons();
            for (size_t iPoly = 0; iPoly < boundingBoxes.size(); iPoly++)
            {
                const auto *polygonName = boundingBoxes[iPoly]->getName().buffer();
                if (polygonName == std::string("non_walkable_area"))
                {
                    // obj->bounds->polygons
                    for (size_t i = 0; i < polygons[iPoly]->_vertices.size(); i += 2)
                    {
                        forbidden_area.emplace_back((polygons[iPoly]->_vertices[i + 0]) + obj->getPosition().x, (polygons[iPoly]->_vertices[i + 1]) + obj->getPosition().y);
                    }
                    // Add first to the back again.
                    forbidden_area.emplace_back((polygons[iPoly]->_vertices[0]) + obj->getPosition().x, (polygons[iPoly]->_vertices[1]) + obj->getPosition().y);
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
    const auto walkableResult = spSkeletonBounds_containsPointMatchingName(bounds.get(), "walkable_area", static_cast<float>(position.x), static_cast<float>(position.y));
    if (!walkableResult)
    {
        return false;
    }

    if (auto _game = game.lock())
    {
        for (const auto &obj : _game->gameObjects)
        {
            auto *non_walkable = spSkeletonBounds_containsPointMatchingName(obj->bounds.get(), "non_walkable_area", static_cast<float>(position.x) - static_cast<float>(obj->getPosition().x), static_cast<float>(position.y) - static_cast<float>(obj->getPosition().y));
            if (non_walkable)
            {
                return false;
            }
        }
    }

    // if there is an interactable region and a walkable spot,
    // just interact, don't walk there
    return spSkeletonBounds_containsPointNotMatchingName(
               bounds.get(), "walkable_area", static_cast<float>(position.x), static_cast<float>(position.y)) == nullptr;
}
