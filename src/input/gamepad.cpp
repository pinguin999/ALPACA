#include "gamepad.hpp"

Gamepad::Gamepad(std::shared_ptr<jngl::Controller> controller)
	: controller(std::move(controller))
{
}

jngl::Vec2 Gamepad::getMovement() const
{
	const jngl::Vec2 dpad(-controller->state(jngl::controller::DpadLeft) +
							  controller->state(jngl::controller::DpadRight),
						  -controller->state(jngl::controller::DpadUp) +
							  controller->state(jngl::controller::DpadDown));
	const jngl::Vec2 sticks(controller->state(jngl::controller::LeftStickX),
							-controller->state(jngl::controller::LeftStickY));
	const jngl::Vec2 sum = dpad + sticks;
	if (boost::qvm::mag_sqr(sum) > 1)
	{
		return boost::qvm::normalized(sum);
	}
	return sum;
}

jngl::Vec2 Gamepad::getMovementStep() const
{
	const jngl::Vec2 dpad(-controller->pressed(jngl::controller::DpadLeft) +
							  controller->pressed(jngl::controller::DpadRight),
						  -controller->pressed(jngl::controller::DpadUp) +
							  controller->pressed(jngl::controller::DpadDown));
	const jngl::Vec2 sticks(controller->pressed(jngl::controller::LeftStickX),
							-controller->pressed(jngl::controller::LeftStickY));
	const jngl::Vec2 sum = dpad + sticks;
	if (boost::qvm::mag_sqr(sum) > 1)
	{
		return boost::qvm::normalized(sum);
	}
	return sum;
}

jngl::Vec2 Gamepad::getSecondaryMovement() const
{
	const jngl::Vec2 sticks(controller->state(jngl::controller::RightStickX),
							-controller->state(jngl::controller::RightStickY));
	if (boost::qvm::mag_sqr(sticks) > 1)
	{
		return boost::qvm::normalized(sticks);
	}
	return sticks;
}

bool Gamepad::primary() const
{
	return controller->pressed(jngl::controller::A);
}

bool Gamepad::secondary() const
{
	return controller->pressed(jngl::controller::B);
}

bool Gamepad::rTrigger() const
{
	return controller->pressed(jngl::controller::RightTrigger);
}

bool Gamepad::lTrigger() const
{
	return controller->pressed(jngl::controller::RightTrigger);
}

void Gamepad::vibrate()
{
	controller->rumble(0.5f, std::chrono::milliseconds(100));
}
