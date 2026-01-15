#include "scene_fade.hpp"

SceneFade::SceneFade(std::shared_ptr<jngl::Work> game,
                     std::function<void()> loadScene)
    : loadScene(std::move(loadScene)), game(std::move(game)) {}

void SceneFade::step() {
    if (loadScene) {
        alpha += 0.05f;
        if (alpha > 1) {
            alpha = 1;
            loadScene();
            loadScene = nullptr;
        }
    } else {
        game->step();
        alpha -= 0.05f;
        if (alpha < 0) {
            alpha = 0;
            jngl::setScene(std::move(game));
        }
    }
}

void SceneFade::draw() const {
    game->draw();
    jngl::setAlpha(static_cast<uint8_t>(std::lround(alpha * 255.f)));
    jngl::drawRect(jngl::modelview().translate(-jngl::getScreenSize()),
                   jngl::getScreenSize() * 2, 0x000000_rgb);
    jngl::setAlpha(255);
}
