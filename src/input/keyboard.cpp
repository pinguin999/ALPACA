#include <jngl.hpp>
#include "keyboard.hpp"


template <class Key>
jngl::Vec2 findMovement(Key left, Key right, Key up, Key down)
{
	jngl::Vec2 direction;
	if (jngl::keyDown(left))
	{
		direction -= jngl::Vec2(1, 0);
	}
	if (jngl::keyDown(right))
	{
		direction += jngl::Vec2(1, 0);
	}

	if (jngl::keyDown(up))
	{
		direction -= jngl::Vec2(0, 1);
	}
	if (jngl::keyDown(down))
	{
		direction += jngl::Vec2(0, 1);
	}
	return direction;
}

template <class Key>
jngl::Vec2 findStepMovement(Key left, Key right, Key up, Key down)
{
	jngl::Vec2 direction;
	if (jngl::keyPressed(left))
	{
		direction -= jngl::Vec2(1, 0);
	}
	if (jngl::keyPressed(right))
	{
		direction += jngl::Vec2(1, 0);
	}

	if (jngl::keyPressed(up))
	{
		direction -= jngl::Vec2(0, 1);
	}
	if (jngl::keyPressed(down))
	{
		direction += jngl::Vec2(0, 1);
	}
	return direction;
}

jngl::Vec2 Keyboard::getMovement() const
{
	// change speed
	jngl::Vec2 direction;
	direction = findMovement('a', 'd', 'w', 's');

	if (boost::qvm::mag_sqr(direction) > 1)
	{
		boost::qvm::normalize(direction);
	}
	return direction;
}

jngl::Vec2 Keyboard::getMovementStep() const
{
	// change speed
	jngl::Vec2 direction;
	direction = findStepMovement('a', 'd', 'w', 's');
	direction += findStepMovement(jngl::key::Left, jngl::key::Right, jngl::key::Up, jngl::key::Down);

	if (boost::qvm::mag_sqr(direction) > 1)
	{
		boost::qvm::normalize(direction);
	}
	return direction;
}

jngl::Vec2 Keyboard::getSecondaryMovement() const
{
	// change speed
	jngl::Vec2 direction;
	direction = findMovement(jngl::key::Left, jngl::key::Right, jngl::key::Up, jngl::key::Down);

	if (boost::qvm::mag_sqr(direction) > 1)
	{
		boost::qvm::normalize(direction);
	}
	return direction;
}

bool Keyboard::primary() const {
    return jngl::keyPressed(jngl::key::Return) || jngl::keyPressed(jngl::key::Space) || jngl::keyPressed('e');
}

bool Keyboard::secondary() const {
    return jngl::keyPressed(jngl::key::BackSpace);
}

bool Keyboard::lTrigger() const
{
	return false;
}

bool Keyboard::rTrigger() const
{
	return false;
}
