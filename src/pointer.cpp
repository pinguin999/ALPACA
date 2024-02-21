#include "pointer.hpp"

#include "input/gamepad.hpp"
#include "input/keyboard.hpp"
#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>
#include <cassert>
#include <spine/spine.h>

Pointer::Pointer(std::shared_ptr<Game> game, const std::string &spine_file) : SpineObject(game, spine_file, "Pointer", .5),
                                                                              GAMEPAD_SPEED_MULTIPLYER(game->config["gamepad_speed_multiplyer"].as<double>()),
                                                                              max_speed(std::abs(game->config["pointer_max_speed"].as<float>())),
                                                                              pointer_idle_animation(game->config["pointer_idle_animation"].as<std::string>()),
                                                                              pointer_over_animation(game->config["pointer_over_animation"].as<std::string>())
{
    const auto controllers = jngl::getConnectedControllers();
    if (controllers.size() > pointerNr)
    {
        control = std::make_unique<Gamepad>(controllers[pointerNr]);
    }
    else
    {
        control = std::make_unique<Keyboard>();
    }
}

Pointer::~Pointer()
{
}

bool Pointer::step(bool)
{
    const auto controllers = jngl::getConnectedControllers();
    if (controllers.size() > 0)
    {
        control = std::make_unique<Gamepad>(controllers[pointerNr]);
    }
    if (auto _game = game.lock())
    {
        jngl::Vec2 screensize = jngl::getScreenSize();

        auto mouse_pose = jngl::getMousePos() + (screensize / 2.0);

        // Mouse position with moved camera
        auto cam_pos = _game->getCameraPosition() - (screensize / 2.0);
        mouse_pose = (mouse_pose + cam_pos) / _game->getCameraZoom();

        auto move = control->getMovement() * GAMEPAD_SPEED_MULTIPLYER;
        auto movesec = control->getSecondaryMovement();
        if (move != jngl::Vec2(0, 0))
        {
            position += move;
            position.x = std::max(position.x, -screensize.x / _game->getCameraZoom() / 2);
            position.y = std::max(position.y, -screensize.y / _game->getCameraZoom() / 2);
            position.x = std::min(position.x, screensize.x / _game->getCameraZoom() / 2);
            position.y = std::min(position.y, screensize.y / _game->getCameraZoom() / 2);
            target_position = position;
        }
        else if (movesec != jngl::Vec2(0, 0))
        {
            position.x = screensize.x * jngl::getScaleFactor() / 2.0 + movesec.x * screensize.x * jngl::getScaleFactor() / 2.0;
            position.y = screensize.y * jngl::getScaleFactor() / 2.0 + movesec.y * screensize.y * jngl::getScaleFactor() / 2.0;
            target_position = position;
        }
        else if (target_position != mouse_pose && last_mouse_pose != mouse_pose)
        {
            target_position = mouse_pose;
            position = mouse_pose;
        }
        else
        {
            jngl::Vec2 tmp_target_position = target_position - position;
            auto magnitude = std::sqrt(boost::qvm::dot(tmp_target_position, tmp_target_position));
            if (magnitude != 0 && magnitude > max_speed)
            {
                tmp_target_position *= max_speed / magnitude;
            }
            position += tmp_target_position;
        }

        bool over = false;
        auto dlgMan = _game->getDialogManager();
        if (dlgMan->isSelectTextActive())
        {
            over = dlgMan->isOverText(position);
        }
        // Region and Object Collision Test nur, wenn kein Dialog läuft.
        else
        {
            for (auto obj : _game->gameObjects)
            {
                if (obj->getVisible() &&
                    !(_game->getInactivLayerBorder() > obj->layer) &&
                    obj->bounds &&
                    bool(spine::spSkeletonBounds_containsPointNotMatchingName(obj->bounds, "walkable_area", (float)position.x - (float)obj->getPosition().x, (float)position.y - (float)obj->getPosition().y)))
                {
                    over = true;
                    vibrate();
                    break;
                }
            }
        }

        if (over)
        {
            if (currentAnimation != pointer_over_animation)
            {
                currentAnimation = pointer_over_animation;
                playAnimation(0, currentAnimation, true, (*_game->lua_state)["pass"]);
                this->setSkin("active");
            }
        }
        else
        {
            if (currentAnimation != pointer_idle_animation)
            {
                currentAnimation = pointer_idle_animation;
                playAnimation(0, currentAnimation, true, (*_game->lua_state)["pass"]);
                this->setSkin("inactive");
            }
        }

        last_mouse_pose = mouse_pose;

        skeleton->step();
    }

    return false;
}

void Pointer::draw() const
{
    jngl::pushMatrix();
    jngl::translate(position);
    jngl::rotate(getRotation());

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->editMode)
        {
            jngl::popMatrix();
            return;
        }
    }
#endif

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        skeleton->debugdraw = _game->enableDebugDraw;
    }
#endif

    skeleton->draw();

    jngl::popMatrix();
}

void Pointer::vibrate()
{
    control->vibrate();
}

jngl::Vec2 Pointer::getMovement() const
{
    return control->getMovement();
}

jngl::Vec2 Pointer::getMovementStep() const
{
    return control->getMovementStep();
}

jngl::Vec2 Pointer::getSecondaryMovement() const
{
    return control->getSecondaryMovement();
}

bool Pointer::primary() const
{
#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->editMode)
        {
            return false;
        }
    }
#endif
    return control->primary();
};

bool Pointer::secondary() const
{
#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->editMode)
        {
            return false;
        }
    }
#endif
    return control->secondary();
};

bool Pointer::primaryPressed()
{
#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->editMode)
        {
            return false;
        }
    }
#endif
    return jngl::mousePressed() || control->primary() || control->rTrigger();
}

bool Pointer::primaryDown()
{
#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->editMode)
        {
            return false;
        }
    }
#endif
    return jngl::mouseDown() || control->primary() || control->rTrigger();
}

bool Pointer::secondaryPressed()
{
#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->editMode)
        {
            return false;
        }
    }
#endif
    return jngl::mousePressed(jngl::mouse::Right) || control->secondary();
}

void Pointer::setPrimaryHandled()
{
    primaryAlreadyHandled = true;
}

bool Pointer::isPrimaryAlreadyHandled()
{
    return primaryAlreadyHandled;
}

void Pointer::setSecondaryHandled()
{
    secondaryAlreadyHandled = true;
}

bool Pointer::isSecondaryAlreadyHandled()
{
    return secondaryAlreadyHandled;
}
void Pointer::resetHandledFlags()
{
    primaryAlreadyHandled = false;
    secondaryAlreadyHandled = false;
}

double Pointer::getMouseWheel()
{
    return jngl::getMouseWheel();
}
