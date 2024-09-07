#pragma once
#include "../spine_object.hpp"

#include <string>

class Game;

class SpeechBubble : public SpineObject
{
public:
    SpeechBubble(std::shared_ptr<Game> game, const std::string &spine_file,
                           const jngl::Text &text,
                           const jngl::Color textColor,
                           jngl::Vec2 pos);
    void draw() const override;
    bool step(bool force = false) override;

private:
    jngl::Color color;
    jngl::Text text;
};
