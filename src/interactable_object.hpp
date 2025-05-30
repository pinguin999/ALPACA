#pragma once

#include "mouse_info.hpp"
#include "spine_object.hpp"

#include <jngl.hpp>
#include <sol/sol.hpp>

class InteractableObject : public SpineObject
{
public:
    explicit InteractableObject(const std::shared_ptr<Game> &game, const std::string &spine_file, const std::string &id, float scale = 1.0);
    ~InteractableObject() override = default;

    bool step(bool force = false) override;

    void draw() const override;

    void registerToDelete();
    void setLuaIndex(const std::string &index) { luaIndex = index; };

private:
    std::string luaIndex;
#ifndef NDEBUG
    bool mouseOver = false;
    std::optional<MouseInfo::Down> mouseDown;
#endif
};
