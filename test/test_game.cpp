
#ifdef STAND_ALONE
#define BOOST_TEST_MODULE PacTests
#endif

#include <boost/ut.hpp>
#include <ctime>
#include <jngl/message.hpp>
#include <jngl/input.hpp>
#include <jngl/job.hpp>
#include <random>

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

#ifndef NDEBUG
void pac_unload_file(const char* path)
{
}
#endif

const int SEED = 0;
const int MAX_STEPS = 10000;


using namespace boost::ut;
suite alpaca_test_suite = []
{
    "game_play_test"_test = []
    {
        jngl::setVolume(0);
        std::mt19937 gen = std::mt19937(SEED);
#ifdef EMSCRIPTEN
        chdir("data");
#elif !defined(ANDROID)
        auto dataFolder = fs::path("../data");
        if (!fs::exists(dataFolder))
        {
            dataFolder = fs::path("../../data");
            if (!fs::exists(dataFolder))
            {
                dataFolder = fs::path("../../../../data");
                if (!fs::exists(dataFolder))
                {
                    dataFolder = fs::path("data");
                }
            }
        }
        fs::current_path(dataFolder);
        jngl::debug(fs::current_path());
#endif
        YAML::Node const config = YAML::Load(jngl::readAsset("config/game.json").str());
        jngl::showWindow((config)["name"].as<std::string>(), 800, 600, 0, {16, 9}, {16, 9});
        jngl::setAntiAliasing(true);

        jngl::writeConfig("savegame", "");

        auto game = std::make_shared<Game>(config);

        game->init();
        game->enable_fade = false;

        std::vector<std::tuple<std::string, std::shared_ptr<SpineObject>>> actions;

        game->step();
        game->step();

        (*game->lua_state)["game_finished"] = false;

        int i = 0;

        while (!(*game->lua_state)["game_finished"] && i < MAX_STEPS)
        {
            jngl::updateInput();
            i++;
            actions.clear();

            game->step();

            // TODO: Solange ein Callback gesetzt ist keine neue Aktion auswählen.
            std::string options = "";
            for (auto &obj : game->gameObjects)
            {
                if (game->getInactivLayerBorder() > obj->layer || obj->getParent() != nullptr)
                {
                    continue;
                }

                if (obj->getVisible() && obj->bounds && obj->bounds->boundingBoxes)
                {
                    for (int j = 0; j < obj->bounds->count; j++)
                    {
                        if (std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "walkable_area" &&
                            std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "setDE" &&
                            std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "setEN")
                        {
                            options += std::string(obj->bounds->boundingBoxes[j]->super.super.name) + ", ";
                            actions.emplace_back(std::string(obj->bounds->boundingBoxes[j]->super.super.name), obj);
                        }
                    }
                }
            }
            jngl::debug("OPTIONS {}", options);


            int const min = 0;
            int const max = actions.size() - 1;
            int const randAction = std::abs(int(gen())) % (max - min + 1) + min;

            jngl::debug("RUN: {}", std::get<0>(actions.at(randAction)));
            game->runAction(std::get<0>(actions.at(randAction)), std::get<1>(actions.at(randAction)));

            // Give Action time
            for (int _i = 0; _i < 400; _i++)
            {
                game->step();
                if (game->getDialogManager()->isActive())
                {
                    if (game->getDialogManager()->isSelectTextActive())
                    {
                        auto choices = game->getDialogManager()->getChoiceTextsSize();
                        int const min = 0;
                        int const max = choices - 1;
                        int const choice = std::abs(int(gen())) % (max - min + 1) + min;
                        game->getDialogManager()->selectCurrentAnswer(choice);
                    }
                    else
                    {
                        game->getDialogManager()->continueCurrent();
                    }
                }
            }
        }
        // expect(eq(game->getInactivLayerBorder(), 2));
        jngl::debug("Took: {} steps", i);

        jngl::hideWindow();

        expect(neq(i, MAX_STEPS));
    };

    "game_save_load_test"_test = []
    {
        return; // DISABLED
        jngl::setVolume(0);
        std::mt19937 gen = std::mt19937(SEED);
#ifdef EMSCRIPTEN
        chdir("data");
#elif !defined(ANDROID)
        auto dataFolder = fs::path("../data");
        if (!fs::exists(dataFolder))
        {
            dataFolder = fs::path("../../data");
            if (!fs::exists(dataFolder))
            {
                dataFolder = fs::path("../../../../data");
                if (!fs::exists(dataFolder))
                {
                    dataFolder = fs::path("data");
                }
            }
        }
        fs::current_path(dataFolder);
#endif
        jngl::showWindow("Test", 800, 600, 0, {16, 9}, {16, 9});
        jngl::setAntiAliasing(true);

        jngl::writeConfig("savegame", "");

        YAML::Node const config = YAML::Load(jngl::readAsset("config/game.json").str());

        int i = 0;
        std::shared_ptr<Game> game;
        do
        {
            game.reset();
            game = std::make_shared<Game>(config);

            game->init();
            game->enable_fade = false;

            std::vector<std::tuple<std::string, std::shared_ptr<SpineObject>>> actions;
            game->step();
            game->step();

            (*game->lua_state)["game_finished"] = false;

            i++;
            actions.clear();

            game->step();

            // TODO: Solange ein Callback gesetzt ist keine neue Aktion auswählen.

            std::string options;
            for (auto &obj : game->gameObjects)
            {
                if (game->getInactivLayerBorder() > obj->layer)
                {
                    continue;
                }

                if (obj->getVisible() && obj->bounds && obj->bounds->boundingBoxes)
                {
                    for (int j = 0; j < obj->bounds->count; j++)
                    {
                        if (std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "walkable_area" &&
                            std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "setDE" &&
                            std::string(obj->bounds->boundingBoxes[j]->super.super.name) != "setEN")
                        {
                            options += std::string(obj->bounds->boundingBoxes[j]->super.super.name) + ", ";
                            actions.emplace_back(std::string(obj->bounds->boundingBoxes[j]->super.super.name), obj);
                        }
                    }
                }
            }
            jngl::debug("OPTIONS: {}", options);

            int const min = 0;
            int const max = actions.size() - 1;
            int const randAction = std::abs(int(gen())) % (max - min + 1) + min;

            jngl::debug("RUN: {}", std::get<0>(actions.at(randAction)));
            game->runAction(std::get<0>(actions.at(randAction)), std::get<1>(actions.at(randAction)));

            // Give Action time
            for (int _i = 0; _i < 400; _i++)
            {
                game->step();
                if (game->getDialogManager()->isActive())
                {
                    if (game->getDialogManager()->isSelectTextActive())
                    {
                        auto choices = game->getDialogManager()->getChoiceTextsSize();
                        int const min = 0;
                        int const max = choices - 1;
                        int const choice = std::abs(int(gen())) % (max - min + 1) + min;
                        game->getDialogManager()->selectCurrentAnswer(choice);
                    }
                    else
                    {
                        game->getDialogManager()->continueCurrent();
                    }
                }
            }

            game->saveLuaState();
        } while (!(*game->lua_state)["game_finished"] && i < MAX_STEPS);

        // expect(eq(game->getInactivLayerBorder(), 2));
        jngl::debug("Took: {} steps", i);

        jngl::hideWindow();

        expect(neq(i, MAX_STEPS));
    };
};
