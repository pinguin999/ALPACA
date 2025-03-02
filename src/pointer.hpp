#pragma once

#include "spine_object.hpp"
#include "input/control.hpp"

#include <jngl.hpp>
#include <memory>

class Pointer : public SpineObject
{
public:
    explicit Pointer(std::shared_ptr<Game> game, const std::string &spine_file);
    ~Pointer() override = default;

    bool step(bool force = false) override;

    void draw() const override;

    void vibrate();

    jngl::Vec2 target_position = jngl::Vec2(0, 1080);
    jngl::Vec2 last_target_position = jngl::Vec2(0, 0);
    bool primaryPressed();
    bool primaryDown();
    void setPrimaryHandled();
    bool isPrimaryAlreadyHandled();

    bool secondaryPressed();
    void setSecondaryHandled();
    bool isSecondaryAlreadyHandled();
    void resetHandledFlags();
    double getMouseWheel();

    jngl::Vec2 getMovement() const;
    jngl::Vec2 getMovementStep() const;
    jngl::Vec2 getSecondaryMovement() const;
    bool primary() const;
    bool secondary() const;

    jngl::Vec2 getWorldPosition();

    std::vector<std::shared_ptr<SpineObject>> attachedObjects;

private:
    bool primaryAlreadyHandled = false;
    bool secondaryAlreadyHandled = false;

    std::unique_ptr<Control> control;

    jngl::Vec2 last_mouse_pose = jngl::Vec2(0, 0);

    size_t pointerNr = 0;
    jngl::Vec2 last_position;
    float fade_out = 2.f;
};
