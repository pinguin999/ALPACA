#pragma once

#include "control.hpp"

class Keyboard : public Control
{
public:
	explicit Keyboard() = default;

	jngl::Vec2 getMovement() const override;
	jngl::Vec2 getMovementStep() const override;
	jngl::Vec2 getSecondaryMovement() const override;
	bool primary() const override;
	bool secondary() const override;
	bool lTrigger() const override;
	bool rTrigger() const override;
};
