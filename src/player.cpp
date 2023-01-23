#include "player.hpp"

#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>

Player::Player(std::shared_ptr<Game> game, const std::string &spine_file) : SpineObject(game, spine_file, "Player", game->config["player_scale"].as<float>()),
                                                                            DOUBLE_CLICK_TIME(game->config["double_click_time"].as<double>()),
                                                                            MAX_CLICK_DISTANCE(game->config["max_click_distance"].as<int>()),
                                                                            NEAR_OBJECT(game->config["near_object"].as<int>()),
                                                                            X_BORDER(game->config["x_border"].as<int>()),
                                                                            max_speed(std::abs(game->config["player_max_speed"].as<float>())),
                                                                            player_walk_animation(game->config["player_walk_animation"].as<std::string>()),
                                                                            player_side_skin(game->config["player_side_skin"].as<std::string>()),
                                                                            player_up_skin(game->config["player_up_skin"].as<std::string>()),
                                                                            player_front_skin(game->config["player_front_skin"].as<std::string>()),
                                                                            player_beam_animation(game->config["player_beam_animation"].as<std::string>()),
                                                                            player_idle_animation(game->config["player_idle_animation"].as<std::string>())
{
    last_click_time = std::numeric_limits<double>::min();
    target_position = jngl::Vec2((*game->lua_state)["player"]["x"], (*game->lua_state)["player"]["y"]);
    position = target_position;
    path.push_back(target_position);
    SpineObject::id = "Player";

    setSkin((*game->lua_state)["player"]["skin"]);
}

Player::~Player()
{
}

void Player::setDirection(jngl::Vec2 target_position)
{
    if (max_speed == 0.0)
    {
        return;
    }

    if (auto _game = game.lock())
    {
        currentAnimation = player_walk_animation;
        (*_game->lua_state)["player"]["animation"] = currentAnimation;
        (*_game->lua_state)["player"]["loop_animation"] = true;
        playAnimation(0, currentAnimation, true, (*_game->lua_state)["pass"]);

        double angle = std::atan2(target_position.y - position.y, target_position.x - position.x)*180/M_PI;

        if( angle < 45 && angle > -45)
        {
                // jngl::debugLn("right");
                setSkin(player_side_skin);

                skeleton->skeleton->scaleX = 1;
                auto slot = spSkeleton_findSlot(skeleton->skeleton, "mouth");
                slot->color.a = 0;

                (*_game->lua_state)["player"]["skin"] = player_side_skin;
        }
        else if (angle < -45 && angle > -135)
        {
            // jngl::debugLn("up");
            setSkin(player_up_skin);
            (*_game->lua_state)["player"]["skin"] = player_up_skin;
        }
        else if (angle < 135 && angle > 45)
        {
            // jngl::debugLn("down");
            setSkin(player_front_skin);

            auto slot = spSkeleton_findSlot(skeleton->skeleton, "mouth");
            slot->color.a = 1;

            (*_game->lua_state)["player"]["skin"] = player_front_skin;
        }
        else if (angle < -135 || angle > 135)
        {
            // jngl::debugLn("left");
            setSkin(player_side_skin);

            if (target_position.y != position.y)
            {
                skeleton->skeleton->scaleX = -1;
            }

            auto slot = spSkeleton_findSlot(skeleton->skeleton, "mouth");
            slot->color.a = 0;

            (*_game->lua_state)["player"]["skin"] = player_side_skin;
        }
    }
}

void Player::addTargetPosition(jngl::Vec2 target)
{
    path.push_back(target);
}

void Player::addTargetPositionImmediately(jngl::Vec2 target, sol::function callback)
{
    if (auto _game = game.lock())
    {
        path.clear();
        if (target != position)
        {
            newPath = _game->currentScene->background->getPathToTarget(position, target);

            path.insert(path.end(), newPath.begin(), newPath.end());
            this->walk_callback = callback;
        }else{
            this->walk_callback = callback;
            walk_callback();
            walk_callback = (*_game->lua_state)["pass"];
        }
    }
}

// void Player::stop_walking()
// {
//     setTargentPosition(position);
// }

bool Player::step(bool)
{
    if (auto _game = game.lock())
    {
#ifndef NDEBUG
        if (!_game->currentScene->background)
        {
            return false;
        }
#endif

        skeleton->step();
        spSkeleton_updateWorldTransform(skeleton->skeleton);
        spSkeletonBounds_update(bounds, skeleton->skeleton, true);

        if (_game->getDialogManager()->isActive())
            return false;
        if (_game->getInactivLayerBorder() > layer)
            return false;

        if (path.size() > 1 && position == path.front())
        {
            path.pop_front();
            setTargentPosition(path.front());
            setDirection(target_position);
        }

        jngl::Vec2 tmp_target_position = target_position - position;
        if (tmp_target_position == jngl::Vec2(0, 0) && currentAnimation == player_walk_animation)
        {
            currentAnimation = player_idle_animation;
            // Callback to Lua
            walk_callback();
            walk_callback = (*_game->lua_state)["pass"];

            if(currentAnimation == player_idle_animation)
            {
                (*_game->lua_state)["player"]["animation"] = currentAnimation;
                (*_game->lua_state)["player"]["loop_animation"] = true;

                playAnimation(0, currentAnimation, true, (*_game->lua_state)["pass"]);
            }
        }

        auto magnitude = std::sqrt(boost::qvm::dot(tmp_target_position, tmp_target_position));
        if (magnitude != 0 && magnitude > max_speed)
        {
            tmp_target_position *= max_speed / magnitude;
        }
        position += tmp_target_position;

        // TODO it's still possible to walk outside of the nav mesh. We have to fix that soon.
        //#ifndef NDEBUG
        //   if (!_game->currentScene->background->is_walkable(position))
        //   {
        //       throw std::runtime_error("Player is in a not Walkable area!");
        //   }
        //#endif

        if (_game->pointer->secondaryPressed())
        {
            // Maybe LUA's DeattatchAllFromPointer here?
            for (auto obj : _game->pointer->attatchedObjects)
            {
                obj->setParent(nullptr);
                obj->setVisible(false);
            }
            _game->pointer->attatchedObjects.clear();
        }

        if (_game->pointer->secondaryPressed())
        {
            // Maybe LUA's DeattatchAllFromPointer here?
            for (auto obj : _game->pointer->attatchedObjects)
            {
                obj->setParent(nullptr);
                obj->setVisible(false);
            }
            _game->pointer->attatchedObjects.clear();
        }

        if (_game->pointer->primaryPressed() && interruptible && !_game->pointer->isPrimaryAlreadyHandled())
        {
            jngl::Vec2 click_position = _game->pointer->getPosition();
            auto collision = spSkeletonBounds_containsPoint(bounds, (float)click_position.x - (float)position.x, (float)click_position.y - (float)position.y);
            if (collision)
            {
                collision_script = collision->super.super.name;

                jngl::debugLn("clicked player");
                _game->pointer->setPrimaryHandled();
                _game->runAction(collision_script, getptr());

            }

            path.clear();
            newPath = _game->currentScene->background->getPathToTarget(position, click_position);
            walk_callback = (*_game->lua_state)["pass"];

            path.insert(path.end(), newPath.begin(), newPath.end());
            if (path.size() > 0)
            {
                setTargentPosition(path.front());
                setDirection(target_position);
            }

            // Handle double click
            double time = jngl::getTime();
            auto click_distance = boost::qvm::dot(last_click_position - click_position, last_click_position - click_position);
            if (max_speed > 0.0 && _game->currentScene->background->is_walkable(click_position) && time - last_click_time < DOUBLE_CLICK_TIME && click_distance < MAX_CLICK_DISTANCE)
            {
                path.clear();
                walk_callback = (*_game->lua_state)["pass"];
                path.push_back(click_position);
                position = click_position;
                setTargentPosition(click_position);
                setDirection(target_position);
                currentAnimation = player_beam_animation;
                (*_game->lua_state)["player"]["animation"] = currentAnimation;
                (*_game->lua_state)["player"]["loop_animation"] = false;
                playAnimation(0, currentAnimation, false, (*_game->lua_state)["pass"]);
                nextAnimation = player_idle_animation;
            }
            last_click_time = time;
            last_click_position = click_position;
        }

        // Move scean if the player is at the border of the screen.
        auto size = jngl::getScreenSize();

        if (position.x + X_BORDER > size.x / 2.0 / _game->getCameraZoom())
        {
            _game->setCameraPosition(jngl::Vec2((position.x + X_BORDER - size.x / 2.0 / _game->getCameraZoom()), 0), 0, 0);
        }
        else if (position.x - X_BORDER < -size.x / 2.0 / _game->getCameraZoom())
        {
            // Move scean if the player is at the border of the screen.
            _game->setCameraPosition(jngl::Vec2((position.x - X_BORDER + size.x / 2.0 / _game->getCameraZoom()), 0), 0, 0);
        }
        else
        {
            _game->setCameraPosition(_game->getCameraOrigin(), 0, 0);
        }
    }

    return false;
}

void Player::draw() const
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
    jngl::popMatrix();
}

void Player::setTargentPosition(jngl::Vec2 position)
{
    if (auto _game = game.lock())
    {
        target_position = position;
        (*_game->lua_state)["player"]["x"] = position.x;
        (*_game->lua_state)["player"]["y"] = position.y;
    }
}
