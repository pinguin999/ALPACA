#pragma once

#include "spine_object.hpp"

#include <array>
#include <jngl.hpp>
#include <sol/sol.hpp>

class InteractableObject : public SpineObject
{
public:
    explicit InteractableObject(const std::shared_ptr<Game> &game, const std::string &spine_file, const std::string &id, float scale = 1.0);
    ~InteractableObject() = default;

    bool step(bool force = false) override;

    void draw() const override;

    void goToPosition(jngl::Vec2 position, const sol::function &callback);

    void registerToDelete();
    void setLuaIndex(const std::string &index) { luaIndex = index; };

private:
    std::string luaIndex = "";
#ifndef NDEBUG
    const float DEBUG_GRAP_DISTANCE;
    jngl::Vec2 dragposition = jngl::Vec2(0, 0);
#endif
};
