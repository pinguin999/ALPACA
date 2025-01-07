#include "mouse_info.hpp"

void MouseInfo::setMousePos(jngl::Vec2 mousePos) {
    enabled = true;
    this->mousePos = mousePos;
}

jngl::Vec2 MouseInfo::Down::newPos() const {
    return parent->mousePos - startReference;
}

bool MouseInfo::Down::released() { // NOLINT
    return !jngl::mouseDown();
}

MouseInfo::Down::Down(MouseInfo& parent, jngl::Vec2 objectPos)
: parent(&parent), startReference(parent.mousePos - objectPos) {
    parent.down = true;
}

MouseInfo::Down::Down(Down&& other) noexcept
: parent(other.parent), startReference(other.startReference) {
    other.parent = nullptr;
}

auto MouseInfo::Down::operator=(Down&& other) noexcept -> Down& {
    parent = other.parent;
    startReference = other.startReference;
    other.parent = nullptr;
    return *this;
}

MouseInfo::Down::~Down() {
    if (parent) {
        parent->down = false;
    }
}

MouseInfo::Over::Over(MouseInfo& parent) : parent(parent) {
}

auto MouseInfo::Over::pressed(jngl::Vec2 objectPos) -> std::optional<Down> {
    parent.enabled = false;
    if (jngl::mousePressed()) {
        return Down{ parent, objectPos };
    }
    return std::nullopt;
}

jngl::Vec2 MouseInfo::Over::pos() const {
    return parent.mousePos;
}

auto MouseInfo::pos() -> std::optional<Over> {
    if (enabled && !down) {
        return Over{ *this };
    }
    return std::nullopt;
}
