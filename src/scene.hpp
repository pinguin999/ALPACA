#pragma once

#include <jngl.hpp>
#include <yaml-cpp/yaml.h>
#include "background.hpp"

class Game;
class InteractableObject;

class LoadException : public std::runtime_error
{
public:
    explicit LoadException(const char *details);
};

class SceneExit
{
};

class Scene
{
public:
    explicit Scene(const std::string &fileName, std::shared_ptr<Game> game);

    void writeToFile();
    void playMusic();
    std::shared_ptr<InteractableObject> createObject(const std::string &spine_file, std::string id, float scale);
    void loadObjects(YAML::Node objects);

    std::string getSceneName(){return fileName;};

    std::shared_ptr<Background> background;
    int left_border = INT_MIN;
    int right_border = INT_MAX;
    int top_border = INT_MIN;
    int bottom_border = INT_MAX;
private:
    std::string fileName;
    YAML::Node json;

    std::optional<std::string> backgroundMusic;
    std::unique_ptr<jngl::Sprite> zBufferMap;

    const std::weak_ptr<Game> game;
};
