#pragma once

#include <jngl/Vec2.hpp>

class Control
{
public:
	virtual ~Control() = default;

	/// Richtungsvektor
	virtual jngl::Vec2 getMovement() const = 0;
	virtual jngl::Vec2 getMovementStep() const = 0;
	virtual jngl::Vec2 getSecondaryMovement() const = 0;
	virtual bool primary() const = 0;
	virtual bool secondary() const = 0;
	virtual bool lTrigger() const = 0;
	virtual bool rTrigger() const = 0;

	virtual void vibrate();
};
