#pragma once
#include "../spine_object.hpp"

#include <string>

class Game;

class SpeechBubble : public SpineObject
{
public:
    SpeechBubble(std::shared_ptr<Game> game, const std::string &spine_file,
                           jngl::Text text,
                           jngl::Text characterName,
                           const jngl::Rgba characterNameColor);
    void draw() const override;
    bool step(bool force = false) override;
private:
    jngl::Text text;
    jngl::Rgba textColor;
    jngl::Text characterName;
    jngl::Rgba characterNameColor;
};
