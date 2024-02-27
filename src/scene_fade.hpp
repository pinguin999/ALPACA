#pragma once

#include <jngl.hpp>

class SceneFade : public jngl::Work {
public:
    SceneFade(std::shared_ptr<jngl::Work> game, std::function<void()> loadScene);

    void step() override;
    void draw() const override;

private:
    std::function<void()> loadScene;
    std::shared_ptr<jngl::Work> game;
    float alpha = 0;
};
