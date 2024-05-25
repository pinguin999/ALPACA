#pragma once

#include <jngl.hpp>
#include <set>
#include <vector>
#include <sol/sol.hpp>
#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
#include <gifanim.h>
#endif
#include "player.hpp"
#include "pointer.hpp"
#include "scene.hpp"
#include "dialog/dialog_manager.hpp"
#include "audio_manager.hpp"

class Game : public jngl::Work, public std::enable_shared_from_this<Game>
{
public:
    explicit Game(const YAML::Node &config);
    ~Game() override;
    void init();
    void loadSceneWithFade(std::string level);
    void setupLuaFunctions();
    void saveLuaState(const std::string &savefile = "savegame");
    void loadLuaState(const std::optional<std::string> &savefile = "savegame");

    void runAction(const std::string &actionName, std::shared_ptr<SpineObject> thisObject);

    void step() override;
    void draw() const override;

#ifndef NDEBUG
    void debugStep();
    bool editMode = false;
    bool enableDebugDraw = false;
    bool enablezMapDebugDraw = false;
    bool show_debug_info = true;

    jngl::Text debug_info;
#endif

    /// Wendet die Kamera auf JNGLs globale ModelView-Matrix an
    void applyCamera() const;

    /// Je kleiner, desto weiter ist die Kamera herausgezoomt. 1 = default, immer größer 0
    double getCameraZoom() const;

    jngl::Vec2 getCameraPosition() const;
    jngl::Vec2 getCameraSpeed() const;
    void setCameraPosition(jngl::Vec2, double deadzoneFactorX, double deadzoneFactorY);
    void setCameraPositionImmediately(jngl::Vec2);

    void stepCamera();
    void triangulateBorder();

    void add(const std::shared_ptr<SpineObject> &obj);
    void remove(const std::shared_ptr<SpineObject> &obj);
    std::string language;

    std::shared_ptr<sol::state> lua_state;
    std::shared_ptr<Player> player = nullptr;
    std::shared_ptr<Pointer> pointer = nullptr;

    std::shared_ptr<DialogManager> getDialogManager();
    AudioManager *getAudioManager();
    void addObjects();
    void removeObjects();

    std::shared_ptr<Scene> currentScene = nullptr;
    bool reload = false;
    void setInactivLayerBorder(int layer)
    {
        inactivLayerBorder = layer;
        (*lua_state)["inactivLayerBorder"] = layer;
    };
    int getInactivLayerBorder() { return inactivLayerBorder; };

    const std::shared_ptr<SpineObject> getObjectById(std::string objectId);
    const std::string getLuaPath(std::string objectId);

    const std::string cleanLuaString(std::string variable);
    YAML::Node config;
    std::vector<std::shared_ptr<SpineObject>> gameObjects;
    bool enable_fade = true;

private:
    void loadScene(const std::string& level);

    std::vector<std::shared_ptr<SpineObject>> needToAdd;
    std::vector<std::shared_ptr<SpineObject>> needToRemove;
    std::string backupLuaTable(const sol::table table, const std::string &parent);
    jngl::Vec2 cameraPosition;
    jngl::Vec2 targetCameraPosition;
    jngl::Vec2 cameraDeadzone;
    double cameraZoom = 1.0;
    int inactivLayerBorder = 0;
    std::shared_ptr<DialogManager> dialogManager = nullptr;
    AudioManager audioManager;

#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
    std::shared_ptr<GifAnim> gifAnimation;
    std::shared_ptr<GifWriter> gifWriter;
    bool recordingGif = false;
    uint8_t *gifBuffer;
    int gifGameFrame;
    double gifTime;
    const int GIF_FRAME_SKIP = 10;
    const int GIF_DOWNSCALE_FACTOR = 2;
#endif
};
