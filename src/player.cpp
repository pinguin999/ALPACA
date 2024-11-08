#include "player.hpp"

#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>

Player::Player(const std::shared_ptr<Game> &game, const std::string &spine_file) : SpineObject(game, spine_file, "player"),
                                                                                   last_click_time(std::numeric_limits<double>::min())
{

}

void Player::setDirection()
{
    if (auto _game = game.lock())
    {
        if ((*_game->lua_state)["config"]["player_max_speed"] == 0.0)
        {
            return;
        }

        if (currentAnimation != (*_game->lua_state)["config"]["player_walk_animation"])
        {
            currentAnimation = (*_game->lua_state)["config"]["player_walk_animation"];
            (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["animation"] = currentAnimation;
            (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["loop_animation"] = true;
            playAnimation(0, currentAnimation, true, (*_game->lua_state)["pass"]);
        }

        const double angle = std::atan2(target_position.y - position.y, target_position.x - position.x) * 180 / M_PI;

        if (angle < 45 && angle > -45)
        {
            // jngl::debugLn("right");
            setSkin((*_game->lua_state)["config"]["player_side_skin"]);

            if (skeleton->skeleton->scaleX < 0)
            {
                skeleton->skeleton->scaleX *= -1.0;
            }
            auto *slot = spSkeleton_findSlot(skeleton->skeleton, "mouth");
            if (slot)
            {
                slot->color.a = 0;
            }

            (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["skin"] = (*_game->lua_state)["config"]["player_side_skin"];
        }
        else if (angle < -45 && angle > -135)
        {
            // jngl::debugLn("up");
            setSkin((*_game->lua_state)["config"]["player_up_skin"]);
            (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["skin"] = (*_game->lua_state)["config"]["player_up_skin"];
        }
        else if (angle < 135 && angle > 45)
        {
            // jngl::debugLn("down");
            setSkin((*_game->lua_state)["config"]["player_front_skin"]);

            auto *slot = spSkeleton_findSlot(skeleton->skeleton, "mouth");
            if (slot)
            {
                slot->color.a = 1;
            }

            (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["skin"] = (*_game->lua_state)["config"]["player_front_skin"];
        }
        else if (angle < -135 || angle > 135)
        {
            // jngl::debugLn("left");
            setSkin((*_game->lua_state)["config"]["player_side_skin"]);

            if (target_position.y != position.y && skeleton->skeleton->scaleX > 0)
            {
                skeleton->skeleton->scaleX *= -1.0;
            }

            auto *slot = spSkeleton_findSlot(skeleton->skeleton, "mouth");
            if (slot)
            {
                slot->color.a = 0;
            }

            (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["skin"] = (*_game->lua_state)["config"]["player_side_skin"];
        }
    }
}

void Player::addTargetPosition(jngl::Vec2 target)
{
    path.push_back(target);
}

void Player::addTargetPositionImmediately(jngl::Vec2 target, const sol::function &callback)
{
    if (auto _game = game.lock())
    {
        if (boost::qvm::mag_sqr(target_position - target) < 5)
        {
            position = target;
        }
        path.clear();
        if (target != position)
        {
            newPath = _game->currentScene->background->getPathToTarget(position, target);

            path.insert(path.end(), newPath.begin(), newPath.end());
            this->walk_callback = callback;
        }
        else
        {
            this->walk_callback = callback;
            walk_callback();
            walk_callback = (*_game->lua_state)["pass"];
        }
    }
}

void Player::stop_walking()
{
    path.clear();
    setTargentPosition(position);
}

bool Player::step(bool /*force*/)
{
    if (auto _game = game.lock())
    {
#ifndef NDEBUG
        if (!_game->currentScene->background)
        {
            return false;
        }
#endif

        if (_game->getDialogManager()->isActive() || _game->getInactivLayerBorder() > layer)
        {
            skeleton->step();
            spSkeleton_update(skeleton->skeleton, 1.0/60.0);
            spSkeleton_updateWorldTransform(skeleton->skeleton, SP_PHYSICS_UPDATE);
            spSkeletonBounds_update(bounds, skeleton->skeleton, true);
            return false;
        }

        if (path.size() > 1 && position == path.front())
        {
            path.pop_front();
            setTargentPosition(path.front());
            setDirection();
        }

        jngl::Vec2 tmp_target_position = target_position - position;
        if (tmp_target_position == jngl::Vec2(0, 0) && currentAnimation == (*_game->lua_state)["config"]["player_walk_animation"])
        {
            currentAnimation = (*_game->lua_state)["config"]["player_idle_animation"];
            // Callback to Lua
            auto old_callback = walk_callback;
            walk_callback(); // walk_callback can be changed in here.
            if (old_callback == walk_callback)
            {
                // only adjust the walk_callback if the Lua script itself hasn't changed it since
                walk_callback = (*_game->lua_state)["pass"];
            }

            if (currentAnimation == (*_game->lua_state)["config"]["player_idle_animation"])
            {
                (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["animation"] = currentAnimation;
                (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["loop_animation"] = true;

                playAnimation(0, currentAnimation, true, (*_game->lua_state)["pass"]);
            }
        }

        float max_speed = (*_game->lua_state)["config"]["player_max_speed"];
        auto magnitude = std::sqrt(boost::qvm::dot(tmp_target_position, tmp_target_position));
        if (magnitude != 0 && magnitude > max_speed)
        {
            tmp_target_position *= max_speed / magnitude;
        }
        position += tmp_target_position;

        spSkeleton_physicsTranslate(skeleton->skeleton, tmp_target_position.x * 2.0, tmp_target_position.y * 2.0);
        skeleton->step();
        spSkeleton_update(skeleton->skeleton, 1.0/60.0);
        spSkeleton_updateWorldTransform(skeleton->skeleton, SP_PHYSICS_UPDATE);
        spSkeletonBounds_update(bounds, skeleton->skeleton, true);

        // TODO it's still possible to walk outside of the nav mesh. We have to fix that soon.
        // #ifndef NDEBUG
        //   if (!_game->currentScene->background->is_walkable(position))
        //   {
        //       throw std::runtime_error("Player is in a not Walkable area!");
        //   }
        // #endif

        if (_game->pointer->secondaryPressed())
        {
            // Maybe Lua's DeattachAllFromPointer here?
            for (const auto &obj : _game->pointer->attachedObjects)
            {
                obj->setParent(nullptr);
                obj->setVisible(false);
            }
            _game->pointer->attachedObjects.clear();
        }

        if (_game->pointer->primaryDown() && !path.empty() && interruptible && !_game->pointer->isPrimaryAlreadyHandled() && walk_callback == (*_game->lua_state)["pass"] )
        {
            const jngl::Vec2 click_position = _game->pointer->getWorldPosition();

            if (boost::qvm::mag_sqr(target_position - click_position) > 5)
            {
                path.clear();
                newPath = _game->currentScene->background->getPathToTarget(position, click_position);

                path.insert(path.end(), newPath.begin(), newPath.end());
                if (!path.empty())
                {
                    setTargentPosition(path.front());
                }
            }
        }

        if (_game->pointer->primaryPressed() && interruptible && !_game->pointer->isPrimaryAlreadyHandled())
        {
            const jngl::Vec2 click_position = _game->pointer->getWorldPosition();
            auto *collision = spSkeletonBounds_containsPoint(bounds,
                                                             static_cast<float>(click_position.x) - static_cast<float>(position.x),
                                                             static_cast<float>(click_position.y) - static_cast<float>(position.y));
            if (collision)
            {
                collision_script = collision->super.super.name;

                jngl::debugLn("clicked player");
                _game->pointer->setPrimaryHandled();
                _game->runAction(collision_script, getptr());
            }

            const double time = jngl::getTime();
            auto click_distance = boost::qvm::dot(last_click_position - click_position, last_click_position - click_position);
            path.clear();

            // Return if players current position is the target position
            double double_click_time = (*_game->lua_state)["config"]["double_click_time"];
            double max_click_distance = (*_game->lua_state)["config"]["max_click_distance"];
            if (boost::qvm::mag_sqr(target_position - click_position) < 5 && (time - last_click_time >= double_click_time || click_distance >= max_click_distance))
            {
                return false;
            }

            newPath = _game->currentScene->background->getPathToTarget(position, click_position);
            walk_callback = (*_game->lua_state)["pass"];

            path.insert(path.end(), newPath.begin(), newPath.end());
            if (!path.empty())
            {
                setTargentPosition(path.front());
            }

            // Handle double click
            if (max_speed > 0.0 && _game->currentScene->background->is_walkable(click_position) && time - last_click_time < double_click_time && click_distance < max_click_distance)
            {
                path.clear();
                walk_callback = (*_game->lua_state)["pass"];
                path.push_back(click_position);
                position = click_position;
                setTargentPosition(click_position);
                currentAnimation = (*_game->lua_state)["config"]["player_beam_animation"];
                (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["animation"] = currentAnimation;
                (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["loop_animation"] = false;
                playAnimation(0, currentAnimation, false, (*_game->lua_state)["pass"]);
                addAnimation(0, (*_game->lua_state)["config"]["player_idle_animation"], true, 0, (*_game->lua_state)["pass"]);
            }
            last_click_time = time;
            last_click_position = click_position;
        }

        _game->setCameraPosition(calcCamPos(), 0, 0);
    }

    return false;
}

void Player::draw() const
{
    auto mv = jngl::modelview().translate(position).rotate(getRotation());
    if (auto _game = game.lock())
    {
        mv.scale(_game->currentScene->getScale(position));
    }

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        skeleton->debugdraw = _game->enableDebugDraw;
    }
#endif

    skeleton->draw(mv);
}

void Player::setTargentPosition(jngl::Vec2 position)
{
    if (auto _game = game.lock())
    {
        target_position = position;
        (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["x"] = position.x;
        (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["y"] = position.y;
    }
}

jngl::Vec2 Player::calcCamPos()
{
    if (auto _game = game.lock())
    {
        // Move scean if the player is at the border of the screen.
        auto size = jngl::getScreenSize();
        jngl::Vec2 camPos = jngl::Vec2(0, 0);

        int border_x = (*_game->lua_state)["config"]["border"]["x"];
        int border_y = (*_game->lua_state)["config"]["border"]["y"];

        if (position.x + border_x > size.x / 2.0 / _game->getCameraZoom())
        {
            camPos.x = position.x + border_x - size.x / 2.0 / _game->getCameraZoom();
        }
        if (position.x - border_x < -size.x / 2.0 / _game->getCameraZoom())
        {
            camPos.x = position.x - border_x + size.x / 2.0 / _game->getCameraZoom();
        }

        if (position.y + border_y > size.y / 2.0 / _game->getCameraZoom())
        {
            camPos.y = position.y + border_y - size.y / 2.0 / _game->getCameraZoom();
        }
        if (position.y - border_y < -size.y / 2.0 / _game->getCameraZoom())
        {
            camPos.y = position.y - border_y + size.y / 2.0 / _game->getCameraZoom();
        }
        return camPos;
    }
    return {0, 0};
}


void Player::toLuaState()
{
    SpineObject::toLuaState();
    if (auto _game = game.lock()) {
        (*_game->lua_state)["scenes"]["cross_scene"]["items"]["player"]["max_speed"] = (*_game->lua_state)["config"]["player_max_speed"];
    }
}

float Player::getMaxSpeed() const
{
    if (auto _game = game.lock()) {
        return (*_game->lua_state)["config"]["player_max_speed"];
    }
    throw std::runtime_error("Couldn't lock game.");
}

void Player::setMaxSpeed(float speed)
{
    if (auto _game = game.lock()) {
        (*_game->lua_state)["config"]["player_max_speed"] = speed;
    }
}
