#include "speech_bubble.hpp"
#include "../game.hpp"
#include <math.h>

SpeechBubble::SpeechBubble(std::shared_ptr<Game> game,
                           const std::string &spine_file,
                           const jngl::Text &text,
                           const jngl::Text &characterName,
                           const jngl::Rgba characterNameColor
                           )
    : SpineObject(game, spine_file, "SpeechBubble", .2),
      text(text),
      textColor(game->getDialogManager()->textToColor((*game->lua_state)["config"]["default_font_color"])),
      characterName(characterName),
      characterNameColor(characterNameColor)
{
    position = jngl::Vec2{0, jngl::getScreenSize().y / 2.0};

}

bool SpeechBubble::step(bool)
{
    skeleton->step();
    return true;
}

void SpeechBubble::draw() const
{
    if (auto _game = game.lock())
    {
    jngl::pushMatrix();
    jngl::translate(position);
    jngl::rotate(getRotation());

#ifndef NDEBUG
    skeleton->debugdraw = _game->enableDebugDraw;
#endif

    skeleton->draw();
    jngl::popMatrix();

    jngl::pushMatrix();
    jngl::translate(jngl::Vec2{jngl::getScreenSize().x / -2.0 + 50, jngl::getScreenSize().y / 3.5});
    jngl::setFontColor(characterNameColor);
    characterName.draw();
    jngl::popMatrix();

    jngl::pushMatrix();
    jngl::translate(jngl::Vec2{jngl::getScreenSize().x / -2.0 + 100, jngl::getScreenSize().y / 3.5 + 80});
    jngl::setFontColor(textColor);
    text.draw();
    jngl::popMatrix();
    }
}
