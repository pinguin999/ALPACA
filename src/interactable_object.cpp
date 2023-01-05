#include "interactable_object.hpp"

#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>

InteractableObject::InteractableObject(std::shared_ptr<Game> game, const std::string &spine_file, std::string id, float scale) :
    SpineObject(game, spine_file, id, scale)
#ifndef NDEBUG
    ,DEBUG_GRAP_DISTANCE(game->config["debug_grap_distance"].as<float>())
#endif
{

}

InteractableObject::~InteractableObject()
{

}

bool InteractableObject::step(bool force)
{
    if (auto _game = game.lock())
    {
        skeleton->step();
        spSkeleton_updateWorldTransform(skeleton->skeleton);
        spSkeletonBounds_update(bounds, skeleton->skeleton, true);

#ifndef NDEBUG
        if (_game->editMode && jngl::mousePressed())
        {
            dragposition = jngl::getMousePos();
        }
        if (_game->editMode && jngl::mouseDown())
        {
            auto mouse_pose = jngl::getMousePos();

            if (std::sqrt((mouse_pose.x - position.x) * (mouse_pose.x - position.x) + (mouse_pose.y - position.y) * (mouse_pose.y - position.y)) < DEBUG_GRAP_DISTANCE)
            {
                position += (jngl::getMousePos() - dragposition) / _game->getCameraZoom();
                jngl::debugLn(position);
                dragposition = jngl::getMousePos();
            }
        }
        if (_game->editMode && jngl::mousePressed())
        {
            dragposition = jngl::getMousePos();
        }
#endif

        if (!force && _game->getDialogManager()->isActive())
            return false;

        if (!force && _game->getInactivLayerBorder() > layer)
            return false;

        // TODO Double Click on Objekts
        if (_game->pointer->primaryPressed() && visible && !_game->pointer->isPrimaryAlreadyHandled())
        {
            jngl::Vec2 click_position = _game->pointer->getPosition();
            auto collision = spSkeletonBounds_containsPoint(bounds, (float)click_position.x - (float)position.x, (float)click_position.y - (float)position.y);
            if (collision)
            {
                collision_script = collision->super.super.name;

                jngl::debug("clicked interactable item ");
                jngl::debugLn(this->bounds->boundingBoxes[0]->super.super.name);
                _game->pointer->setPrimaryHandled();
                _game->runAction(collision_script, getptr());
            }
        }

        if(parent)
        {
            position = parent->getPosition();
        }
    }

    return deleted;
}

void InteractableObject::registerToDelete()
{
    deleted = true;

    if (auto _game = game.lock())
    {
        std::string scene = _game->cleanLuaString((*_game->lua_state.get())["game"]["scene"]);
        (*_game->lua_state.get())["scenes"][scene]["items"][luaIndex] = sol::lua_nil;
    }
}

void InteractableObject::draw() const
{
    if (!visible)
    {
        return;
    }

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
        if (_game->editMode)
        {
            jngl::drawCircle(jngl::Vec2(0, 0), DEBUG_GRAP_DISTANCE);
        }
    }
#endif

    jngl::popMatrix();
}

void InteractableObject::goToPosition(jngl::Vec2 position, sol::function callback)
{
    if (auto _game = game.lock())
    {
        _game->player->addTargetPositionImmediately(this->position + position, callback);
    }
}
