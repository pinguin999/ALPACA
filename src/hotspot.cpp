#include "hotspot.hpp"

#include "game.hpp"
#include "skeleton_drawable.hpp"

#include <spine/spine.h>

Hotspot::Hotspot(const std::shared_ptr<Game>& game, const std::string& spine_file)
: SpineObject(game, spine_file, "hotspot", .5) {
}

bool Hotspot::step(bool) {
    skeleton->step();

    return false;
}

void Hotspot::draw() const {
    skeleton->draw(jngl::modelview().translate(position));
}

void Hotspot::draw(jngl::Mat3 mv) const {
    skeleton->draw(mv.translate(position));
}
