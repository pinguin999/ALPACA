#pragma once

#include "jngl/Mat3.hpp"
#include "spine_object.hpp"
#include "input/control.hpp"

#include <jngl.hpp>
#include <memory>

class Hotspot : public SpineObject
{
public:
    explicit Hotspot(std::shared_ptr<Game> game, const std::string &spine_file);
    ~Hotspot() override = default;

    bool step(bool force = false) override;

    void draw() const override;
    void draw(jngl::Mat3 mv) const;



private:

};
