#include "interactable_object.hpp"

#include "jngl/Mat3.hpp"
#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>

InteractableObject::InteractableObject(const std::shared_ptr<Game> &game, const std::string &spine_file, const std::string &id, float scale) : SpineObject(game, spine_file, id, scale), luaIndex(id)
{
}

bool InteractableObject::step(bool force)
{
    if (auto _game = game.lock())
    {
        skeleton->step();
        spSkeleton_update(skeleton->skeleton, 1.0f/60.0f);
        spSkeleton_updateWorldTransform(skeleton->skeleton, SP_PHYSICS_UPDATE);
        spSkeletonBounds_update(bounds, skeleton->skeleton, 1);

#ifndef NDEBUG
        if (_game->editMode  && !abs_position)
        {
            mouseOver = false;
            if (std::optional<MouseInfo::Over> over = _game->mouseInfo.pos()) {
                double DEBUG_GRAP_DISTANCE = (*_game->lua_state)["config"]["debug_grap_distance"];
                mouseOver = std::sqrt((over->pos().x - position.x) * (over->pos().x - position.x) +
                                      (over->pos().y - position.y) * (over->pos().y - position.y)) <
                            DEBUG_GRAP_DISTANCE;
                if (mouseOver) {
                    mouseDown = over->pressed(position);
                }
            }
        }
        if (mouseDown) {
            position = mouseDown->newPos();
            jngl::debug("{}", position);
            _game->currentScene->updateObjectPosition(id, position);
            if (mouseDown->released()) {
                mouseDown = std::nullopt;
            }
        }
#endif

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

        // TODO Double Click on Objekts
        if (_game->pointer->primaryPressed() && visible && !_game->pointer->isPrimaryAlreadyHandled())
        {
            jngl::Vec2 click_position = _game->pointer->getWorldPosition();
            if (abs_position)
            {
                click_position = _game->pointer->getPosition();
            }

            auto *collision = spSkeletonBounds_containsPoint(bounds, static_cast<float>(click_position.x) - static_cast<float>(position.x), static_cast<float>(click_position.y) - static_cast<float>(position.y));
            if (collision)
            {
                collision_script = collision->super.super.name;
                if (collision_script != "non_walkable_area") {
                    jngl::debug("clicked interactable item {}", collision_script);
                    _game->pointer->setPrimaryHandled();
                    _game->runAction(collision_script, getptr());
                }
            }
        }

        if (parent)
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
        auto lua_object = _game->getLuaPath(luaIndex);
        _game->lua_state->script(lua_object + " = nil");
    }
}

void InteractableObject::draw() const
{
    if (!visible)
    {
        return;
    }

    auto mv = jngl::modelview();
    if (abs_position)
    {
        mv = jngl::Mat3();
        if (auto _game = game.lock())
        {
            mv.scale(_game->getCameraZoom());
        }
    }
    else
    {
        mv.translate(position);
    }
    mv.rotate(getRotation());

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        skeleton->debugdraw = _game->enableDebugDraw;
    }
#endif

    skeleton->draw(mv);

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
        if (_game->editMode && !abs_position)
        {
            const float DEBUG_GRAP_DISTANCE = (*_game->lua_state)["config"]["debug_grap_distance"];
            jngl::drawCircle(mv, DEBUG_GRAP_DISTANCE,
                             jngl::Rgba(0, mouseOver ? 0.7 : (mouseDown ? 0.4 : 0.9), 0, 0.9));
            jngl::Text pposition;
            pposition.setText("x: " + std::to_string(std::lround(position.x)) +
                              "\ny: " + std::to_string(std::lround(position.y)));
            jngl::setFontColor(jngl::Rgba(1.0, 0, 0, 1.0));
            pposition.setAlign(jngl::Alignment::CENTER);
            pposition.setCenter(0, 0);
            pposition.draw(mv);
        }
    }
#endif
}
