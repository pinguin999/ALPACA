
#ifdef STAND_ALONE
#define BOOST_TEST_MODULE PacTests
#endif

#include <boost/ut.hpp>
#include <ctime>
#include <jngl/message.hpp>
#include <jngl/input.hpp>
#include <jngl/job.hpp>

#include "../src/game.hpp"
#include "../src/interactable_object.hpp"

#ifndef EMSCRIPTEN
#if (defined(__linux__) && !__has_include(<filesystem>))
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif

using namespace boost::ut;
suite alpaca_test_suite = [] {

"mushroom_constructor_test"_test = []
{
#ifdef EMSCRIPTEN
    chdir("data");
#elif !defined(ANDROID)
    auto dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("../data");
    if (!fs::exists(dataFolder))
    {
        dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("../../data");
        if (!fs::exists(dataFolder))
        {
            dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("data");
        }
    }
    fs::current_path(dataFolder);
#endif
    jngl::showWindow("Test", 800, 600, 0, {16, 9}, {16, 9});
    jngl::setAntiAliasing(true);

    YAML::Node config = YAML::Load(jngl::readAsset("config/game.json").str());
    auto game = std::make_shared<Game>(config);
    auto mushroom = InteractableObject(game, "mushroom", "mushroomID");

    jngl::hideWindow();

};

"player_constructor_test"_test = []
{
#ifdef EMSCRIPTEN
    chdir("data");
#elif !defined(ANDROID)
    auto dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("../data");
    if (!fs::exists(dataFolder))
    {
        dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("../../data");
        if (!fs::exists(dataFolder))
        {
            dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("data");
        }
    }
    fs::current_path(dataFolder);
#endif
    jngl::showWindow("Test", 800, 600, 0, {16, 9}, {16, 9});
    jngl::setAntiAliasing(true);

    YAML::Node config = YAML::Load(jngl::readAsset("config/game.json").str());
    auto game = std::make_shared<Game>(config);

    (*game->lua_state)["player"] = game->lua_state->create_table_with(
        "x", 0,
        "y", 0,
        "skin", "front");

    auto player = Player(game, "joy");

    jngl::hideWindow();

};

"game_play_test"_test = []
{
    std::srand(std::time(nullptr));
#ifdef EMSCRIPTEN
    chdir("data");
#elif !defined(ANDROID)
    auto dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("../data");
    if (!fs::exists(dataFolder))
    {
        dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("../../data");
        if (!fs::exists(dataFolder))
        {
            dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("data");
        }
    }
    fs::current_path(dataFolder);
#endif
    jngl::showWindow("Test", 800, 600, 0, {16, 9}, {16, 9});
    jngl::setAntiAliasing(true);

    jngl::writeConfig("savegame", "");

    YAML::Node config = YAML::Load(jngl::readAsset("config/game.json").str());
    auto game = std::make_shared<Game>(config);

    game->init();

    std::vector<std::tuple<std::string, std::shared_ptr<SpineObject>>> actions;

    game->step();
    game->step();
    expect(eq(game->getInactivLayerBorder(), 20));

    (*game->lua_state)["game_finished"] = false;

    int i = 0;

    while(!(*game->lua_state)["game_finished"] && i < 3000)
    {
        i++;
        actions.clear();

        game->step();

        // TODO: Solange ein Callback gesetzt ist keine neue Aktion auswählen.

        jngl::debug("OPTIONS ");
        for (auto& obj : game->gameObjects)
        {
            if(game->getInactivLayerBorder() > obj->layer || obj->getParent() != nullptr)
                continue;

            if(obj->getVisible() && obj->bounds && obj->bounds->boundingBoxes)
            {
                for(int j = 0; j < obj->bounds->count; j++)
                {
                    if(std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "walkable_area"
                        && std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "seastar_inventory"
                        )
                    {
                        jngl::debug(obj->bounds->boundingBoxes[j]->super.super.name);
                        jngl::debug(", ");
                        actions.push_back(std::make_tuple(std::string(obj->bounds->boundingBoxes[j]->super.super.name), obj));
                    }
                }
            }
        }
        jngl::debugLn("");

        int min = 0;
        int max = actions.size() - 1;
        int randAction = rand()%(max-min+1)+min;

        jngl::debug("RUN: ");
        jngl::debugLn(std::get<0>(actions.at(randAction)));
        game->runAction(std::get<0>(actions.at(randAction)), std::get<1>(actions.at(randAction)));

        // Give Action time
        for(int _i = 0; _i < 400; _i++)
        {
            game->step();
            if(game->getDialogManager()->isActive())
            {
                game->getDialogManager()->continueCurrent();
            }
        }
    }
    // expect(eq(game->getInactivLayerBorder(), 2));
    jngl::debug("Took: ");
    jngl::debug(i);
    jngl::debugLn(" steps");

    jngl::hideWindow();

    expect(neq(i, 3000));
};



"game_save_load_test"_test = []
{
    return; // DISABLED
    std::srand(std::time(nullptr));
#ifdef EMSCRIPTEN
    chdir("data");
#elif !defined(ANDROID)
    auto dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("../data");
    if (!fs::exists(dataFolder))
    {
        dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("../../data");
        if (!fs::exists(dataFolder))
        {
            dataFolder = fs::path(jngl::getBinaryPath()) / fs::path("data");
        }
    }
    fs::current_path(dataFolder);
#endif
    jngl::showWindow("Test", 800, 600, 0, {16, 9}, {16, 9});
    jngl::setAntiAliasing(true);

    jngl::writeConfig("savegame", "");

    YAML::Node config = YAML::Load(jngl::readAsset("config/game.json").str());

    int i = 0;
    std::shared_ptr<Game> game;
    do {
        game = std::make_shared<Game>(config);

        game->init();

        std::vector<std::tuple<std::string, std::shared_ptr<SpineObject>>> actions;
        game->step();
        game->step();

        (*game->lua_state)["game_finished"] = false;


        i++;
        actions.clear();

        game->step();

        // TODO: Solange ein Callback gesetzt ist keine neue Aktion auswählen.

        jngl::debug("OPTIONS ");
        for (auto& obj : game->gameObjects)
        {
            if(game->getInactivLayerBorder() > obj->layer)
                continue;

            if(obj->getVisible() && obj->bounds && obj->bounds->boundingBoxes)
            {
                for(int j = 0; j < obj->bounds->count; j++)
                {
                    if(std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "walkable_area")
                    {
                        jngl::debug(obj->bounds->boundingBoxes[j]->super.super.name);
                        jngl::debug(", ");
                        actions.push_back(std::make_tuple(std::string(obj->bounds->boundingBoxes[j]->super.super.name), obj));
                    }
                }
            }
        }

        jngl::debugLn("");

        expect(le(2, actions.size()));

        int min = 0;
        int max = actions.size() - 1;
        int randAction = rand()%(max-min+1)+min;

        jngl::debug("RUN: ");
        jngl::debugLn(std::get<0>(actions.at(randAction)));
        game->runAction(std::get<0>(actions.at(randAction)), std::get<1>(actions.at(randAction)));

        // Give Action time
        for(int _i = 0; _i < 400; _i++)
        {
            game->step();
            if(game->getDialogManager()->isActive())
            {
                game->getDialogManager()->continueCurrent();
            }
        }

        game->saveLuaState();
    } while (!(*game->lua_state)["game_finished"] && i < 3000);


    // expect(eq(game->getInactivLayerBorder(), 2));
    jngl::debug("Took: ");
    jngl::debug(i);
    jngl::debugLn(" steps");

    jngl::hideWindow();

    expect(neq(i, 3000));
};

};
