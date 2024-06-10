#pragma once

#include "spine_object.hpp"

#include <array>
#include <deque>
#include <jngl.hpp>
#include <sol/sol.hpp>

class Player : public SpineObject
{
public:
    explicit Player(const std::shared_ptr<Game> &game, const std::string &spine_file);
    ~Player() override = default;

    bool step(bool force) override;

    void draw() const override;

    void addTargetPosition(jngl::Vec2 target);
    void addTargetPositionImmediately(jngl::Vec2 target, const sol::function &callback);
    void stop_walking();

    float getMaxSpeed() const;
    void setMaxSpeed(float speed);
    jngl::Vec2 calcCamPos();

    bool interruptible = true;
    void toLuaState();

private:
    std::deque<jngl::Vec2> path;
    jngl::Vec2 target_position = jngl::Vec2(0, 0);
    void setTargentPosition(jngl::Vec2 position);
    jngl::Vec2 last_click_position = jngl::Vec2(0, 0);

    double last_click_time = 0;

    void setDirection();

    std::deque<jngl::Vec2> newPath;
};
