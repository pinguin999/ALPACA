#pragma once

#include "control.hpp"

#include <jngl/Controller.hpp>

class Gamepad : public Control
{
public:
	Gamepad(std::shared_ptr<jngl::Controller>);
	jngl::Vec2 getMovement() const override;
	jngl::Vec2 getMovementStep() const override;
	jngl::Vec2 getSecondaryMovement() const override;
	bool primary() const override;
	bool secondary() const override;
	bool lTrigger() const override;
	bool rTrigger() const override;

	void vibrate() override;

private:
	std::shared_ptr<jngl::Controller> controller;
};
