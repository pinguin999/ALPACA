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
    explicit Scene(const std::string &fileName, const std::shared_ptr<Game> &game);

    void playMusic();
    std::shared_ptr<InteractableObject> createObject(const std::string &spine_file, const std::string &id, float scale);
    void createObjectJSON(const YAML::Node &object);
    void createObjectLua(std::string id, std::string scene);
    void loadObjects(YAML::Node objects);

    std::string getSceneName(){return fileName;};
    double getScale(jngl::Vec2 position);

    std::shared_ptr<Background> background;
    int left_border = INT_MIN;
    int right_border = INT_MAX;
    int top_border = INT_MIN;
    int bottom_border = INT_MAX;

    std::unique_ptr<jngl::ImageData> zBufferMap;
#ifndef NDEBUG
    void writeToFile();
    void updateObjectPosition(const std::string &id, jngl::Vec2 position);
#endif
private:
    std::string fileName;
    YAML::Node json;

    std::optional<std::string> backgroundMusic;

    const std::weak_ptr<Game> game;
};
