#include "hotspot.hpp"

#include "input/gamepad.hpp"
#include "input/keyboard.hpp"
#include "jngl/Mat3.hpp"
#include "jngl/matrix.hpp"
#include "skeleton_drawable.hpp"
#include "game.hpp"

#include <cmath>
#include <spine/spine.h>

Hotspot::Hotspot(std::shared_ptr<Game> game, const std::string &spine_file) : SpineObject(game, spine_file, "hotspot", .5)
{

}


bool Hotspot::step(bool)
{
    skeleton->step();


    return false;
}

void Hotspot::draw() const
{
    skeleton->draw(jngl::modelview().translate(position));
}

void Hotspot::draw(jngl::Mat3 mv) const
{
    skeleton->draw(mv.translate(position));
}
