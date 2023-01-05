#include "speech_bubble.hpp"
#include "../game.hpp"
#include <math.h>

SpeechBubble::SpeechBubble(std::shared_ptr<Game> game, const std::string &spine_file,
                           const jngl::Text &text,
                           jngl::Vec2 pos) : SpineObject(game, spine_file, "SpeechBubble", .2),
    text(text)
{
    position = pos;
}

bool SpeechBubble::step(bool)
{
    auto middlebone = spSkeleton_findBone(skeleton->skeleton, "middlemiddle");

    auto textSize = text.getSize();

    if (auto _game = game.lock())
    {
        middlebone->scaleX = textSize.y / _game->config["speechbubbleScaleX"].as<double>();
        middlebone->scaleY = textSize.x / _game->config["speechbubbleScaleY"].as<double>();
    }

    skeleton->step();
    return true;
}

void SpeechBubble::draw() const
{
    jngl::pushMatrix();
    jngl::translate(position);
    jngl::rotate(getRotation());

#ifndef NDEBUG
    if (auto _game = game.lock())
    {
    skeleton->debugdraw = _game->enableDebugDraw;
    }
#endif

    skeleton->draw();
    auto textSize = text.getSize();
    jngl::translate(-textSize.x / 2, -textSize.y / 2);
    jngl::setFontColor(255, 255, 255);
    text.draw();
    jngl::popMatrix();
}
