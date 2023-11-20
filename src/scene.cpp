#include "scene.hpp"

#include <fstream>
#include <streambuf>
#include "interactable_object.hpp"
#include "game.hpp"
#include "player.hpp"


LoadException::LoadException(const char *details)
    : std::runtime_error(details)
{
}

Scene::Scene(const std::string &fileName, std::shared_ptr<Game> game) : json(YAML::Load(jngl::readAsset("scenes/" + fileName + ".json").str())), game(game)
{
    this->fileName = fileName;
    std::string scene = fileName;

    if (json.IsNull())
    {
        jngl::debugLn("Wasn't able to load " + fileName);
        background = nullptr;
        return;
    }

    if (!json.IsMap())
    {
        throw LoadException("Invalid JSON for Scene, expected a JSON object");
    }

    // Get old scene and set game.scene to current
    if (!(*game->lua_state)["game"].valid())
    {
		(*game->lua_state).script("game = {}");
    }
	std::string old_scene = "";
	if ((*game->lua_state)["game"]["scene"].valid())
    {
       	old_scene = (*game->lua_state)["game"]["scene"];
    }
	(*game->lua_state).script("game.scene = '" + fileName + "'");


    if (!(*game->lua_state)["scenes"].valid())
    {
        (*game->lua_state)["scenes"] = game->lua_state->create_table();
    }

    if (!(*game->lua_state)["scenes"][scene].valid())
    {
        (*game->lua_state)["scenes"][scene] = game->lua_state->create_table();
    }

#ifndef NDEBUG
    std::string old_hash = "";
    if ((*game->lua_state)["scenes"][scene]["hash"].valid())
        old_hash = (*game->lua_state)["scenes"][scene]["hash"];
    if(json["hash"].IsDefined() && !json["hash"].IsNull())
    {
        const std::string new_hash = json["hash"].as<std::string>();
        if (old_hash != new_hash)
            (*game->lua_state)["scenes"][scene] = game->lua_state->create_table();
        (*game->lua_state)["scenes"][scene]["hash"] = new_hash;
    }
#endif

    if (json["left_border"].IsDefined() && !json["left_border"].IsNull())
    {
        left_border = json["left_border"].as<int>();
        (*game->lua_state)["scenes"][scene]["left_border"] = left_border;
    }
    if (json["right_border"].IsDefined() && !json["right_border"].IsNull())
    {
        right_border = json["right_border"].as<int>();
        (*game->lua_state)["scenes"][scene]["right_border"] = right_border;
    }
    if (json["top_border"].IsDefined() && !json["top_border"].IsNull())
    {
        top_border = json["top_border"].as<int>();
        (*game->lua_state)["scenes"][scene]["top_border"] = top_border;
    }
    if (json["bottom_border"].IsDefined() && !json["bottom_border"].IsNull())
    {
        bottom_border = json["bottom_border"].as<int>();
        (*game->lua_state)["scenes"][scene]["bottom_border"] = bottom_border;
    }

    if ((*game->lua_state)["scenes"][scene]["background"].valid())
    {
        background = std::make_shared<Background>(game, (*game->lua_state)["scenes"][scene]["background"]["spine"]);
        background->setPosition(jngl::Vec2(0, 0));
        background->layer = 0;
        if ((*game->lua_state)["scenes"][scene]["background"]["skin"].valid())
        {
            std::string skin = (*game->lua_state)["scenes"][scene]["background"]["skin"];

            background->setSkin(skin);
        }
        std::string animation = (*game->lua_state)["scenes"][scene]["background"]["animation"];
        bool loop_animation = (*game->lua_state)["scenes"][scene]["background"]["loop_animation"];

        background->playAnimation(0, animation, loop_animation, (*game->lua_state)["pass"]);
        game->add(background);
    }
    else if (json["background"].IsDefined() && !json["background"].IsNull())
    {
        std::string animation = game->config["background_default_animation"].as<std::string>();
        std::string spine = json["background"]["spine"].as<std::string>();
        if (!(*game->lua_state)["scenes"][scene]["background"].valid())
        {
            (*game->lua_state)["scenes"][scene]["background"] = game->lua_state->create_table_with(
            "animation", animation,
            "loop_animation", true,
            "spine", spine,
            "x", 0,
            "y", 0,
            "visible", true,
            "layer", 0);
        }
        background = std::make_shared<Background>(game, spine);
        background->setPosition(jngl::Vec2(0, 0));
        background->layer = 0;
        if (json["background"]["skin"])
        {
            std::string skin = json["background"]["skin"].as<std::string>();
            (*game->lua_state)["scenes"][scene]["background"]["skin"] = skin;

            background->setSkin(skin);
        }
        game->add(background);
    }

    if (json["zBufferMap"].IsDefined() && !json["zBufferMap"].IsNull())
    {
        this->zBufferMap = std::make_unique<jngl::Sprite>(jngl::Sprite(json["zBufferMap"].as<std::string>()));
    }

    if (json["backgroundMusic"].IsDefined() && !json["backgroundMusic"].IsNull())
    {
        this->backgroundMusic = json["backgroundMusic"].as<std::string>();
    }
    else
    {
        this->backgroundMusic = {};
    }

    if (!(*game->lua_state)["inactivLayerBorder"].valid())
    {
        game->setInactivLayerBorder(0);
    }

    if (json["items"].IsDefined() && !json["items"].IsNull())
    {
        this->loadObjects(json["items"]);
    }

    // Move cross_scene object's Lua from old to new scene
    for (auto it = game->gameObjects.begin(); it != game->gameObjects.end();)
	{
		if ((*it)->cross_scene && (*game->lua_state)["scenes"][old_scene]["items"][(*it)->getId()].valid())
		{
            (*game->lua_state).script("scenes[game.scene].items." + (*it)->getId() + " = " + "scenes[\"" + old_scene + "\"].items." + (*it)->getId());
            (*game->lua_state).script("scenes[\"" + old_scene + "\"].items." + (*it)->getId() + " = nil");
		}
        ++it;
    }

    if (!(*game->lua_state)["player"].valid())
    {
        // TODO der Player sollte hier nicht so eine extra behandlung bekommen.
        std::string animation = game->config["player_start_animation"].as<std::string>();
        (*game->lua_state)["player"] = game->lua_state->create_table_with(
            "animation", animation,
            "loop_animation", true,
            "max_speed", game->config["player_max_speed"].as<float>(),
            "spine", game->config["player"].as<std::string>(),
            "skin", game->config["player_default_skin"].as<std::string>(),
            "x", game->config["player_start_position"]["x"].as<int>(),
            "y", game->config["player_start_position"]["y"].as<int>(),
            "visible", true,
            "cross_scene", true,
            "layer", 1,
            "scaleX", 1,
            "scaleY", 1);

        if (game->player == nullptr)
        {
            game->player = std::make_shared<Player>(game, game->config["player"].as<std::string>());
            game->player->cross_scene = true;
            game->player->playAnimation(0, animation, true, (*game->lua_state)["pass"]);
            (*game->lua_state)["player"]["object"] = std::static_pointer_cast<SpineObject>(game->player);

            game->add(game->player);
        }
    }else{
        if (game->player == nullptr)
        {
            game->player = std::make_shared<Player>(game, (*game->lua_state)["player"]["spine"]);
            game->player->playAnimation(0, (*game->lua_state)["player"]["animation"], (*game->lua_state)["player"]["loop_animation"], (*game->lua_state)["pass"]);
            game->player->setPosition(jngl::Vec2((*game->lua_state)["player"]["x"], (*game->lua_state)["player"]["y"]));
            game->player->setVisible((*game->lua_state)["player"]["visible"]);
            game->player->setMaxSpeed((*game->lua_state)["player"]["max_speed"]);
            float layer = (*game->lua_state)["player"]["layer"];
            game->player->layer = int(layer);
            game->player->cross_scene = (*game->lua_state)["player"]["cross_scene"];

            (*game->lua_state)["player"]["object"] = std::static_pointer_cast<SpineObject>(game->player);

            game->add(game->player);
        }
    }

    if (!(*game->lua_state)["inventory_items"].valid())
        (*game->lua_state)["inventory_items"] = game->lua_state->create_table();

    // TODO CLean up. Code Duplikat von weiter unten
    // Load from Lua and not from json
    sol::table objects = (*game->lua_state)["inventory_items"];
    for (const auto& key_value_pair : objects)
    {
        sol::object key = key_value_pair.first;
        sol::object value = key_value_pair.second;

        std::string id = key.as<std::string>();

        if ((*game->lua_state)["inventory_items"][id]["spine"].valid() &&
            (*game->lua_state)["inventory_items"][id]["x"].valid() &&
            (*game->lua_state)["inventory_items"][id]["y"].valid())
        {
            auto interactable = std::make_shared<InteractableObject>(game, (*game->lua_state)["inventory_items"][id]["spine"], id, (*game->lua_state)["inventory_items"][id]["scale"]);

            std::string x = (*game->lua_state)["inventory_items"][id]["x"];
            std::string y = (*game->lua_state)["inventory_items"][id]["y"];

            std::string animation = (*game->lua_state)["inventory_items"][id]["animation"];
            bool loop_animation = (*game->lua_state)["inventory_items"][id]["loop_animation"];

            interactable->playAnimation(0, animation, loop_animation, (*game->lua_state)["pass"]);
            interactable->setPosition(jngl::Vec2(std::stof(x), std::stof(y)));
            interactable->setVisible(false);
            interactable->cross_scene = true;
            interactable->setLuaIndex(id);

            (*game->lua_state)["inventory_items"][id]["object"] = std::static_pointer_cast<SpineObject>(interactable);

            if ((*game->lua_state)["inventory_items"][id]["skin"].valid())
            {
                std::string skin = (*game->lua_state)["inventory_items"][id]["skin"];

                interactable->setSkin(skin);
            }
            game->add(interactable);
        }
    }
}


void Scene::playMusic()
{
    if (auto _game = game.lock())
    {
        if(backgroundMusic.has_value())
        {
            _game->getAudioManager()->loopMusic(backgroundMusic.value());
        }
        else
        {
            _game->getAudioManager()->stopMusic();
        }
    }
}

void Scene::loadObjects(YAML::Node objects)
{
    if (auto _game = game.lock())
    {
        std::string scene = _game->cleanLuaString((*_game->lua_state)["game"]["scene"]);

        if (!(*_game->lua_state)["scenes"][scene]["items"].valid())
        {
            (*_game->lua_state)["scenes"][scene]["items"] = _game->lua_state->create_table();

            for (YAML::const_iterator object = objects.begin(); object != objects.end(); ++object)
            {
                std::string spine_file = (*object)["spine"].as<std::string>();
                std::string object_id;
                if((*object)["id"])
                {
                    object_id = (*object)["id"].as<std::string>();
                }else
                {
                    // Fallback to spine file name if id is not set
                    object_id = spine_file;
                }

                float scale = (*object)["scale"].as<float>(1);
                int layer = (*object)["layer"].as<int>(1);
                std::string animation = (*object)["animation"].as<std::string>("");
                bool cross_scene = (*object)["cross_scene"].as<bool>(false);
                bool abs_position = (*object)["abs_position"].as<bool>(false);
                bool visible = (*object)["visible"].as<bool>(true);

                auto interactable = std::make_shared<InteractableObject>(_game, spine_file, object_id, scale);
                interactable->layer = layer;
                if (animation != "")
                {
                    interactable->playAnimation(0, animation, true, (*_game->lua_state)["pass"]);
                }else{
                    animation = _game->config["spine_default_animation"].as<std::string>();
                }

                interactable->setPosition(jngl::Vec2((*object)["x"].as<float>(), (*object)["y"].as<float>()));
                interactable->setLuaIndex(object_id);
                interactable->cross_scene = cross_scene;
                interactable->abs_position = abs_position;
                interactable->setVisible(visible);

                (*_game->lua_state)["scenes"][scene]["items"][object_id] = _game->lua_state->create_table_with(
                    "spine", spine_file,
                    "object", std::static_pointer_cast<SpineObject>(interactable),
                    "x", std::to_string((*object)["x"].as<float>()),
                    "y", std::to_string((*object)["y"].as<float>()),
                    "animation", animation,
                    "loop_animation", true,
                    "visible", visible,
                    "cross_scene", cross_scene,
                    "abs_position", abs_position,
                    "layer", layer,
                    "scale", scale);

                if ((*object)["skin"])
                {
                    std::string skin = (*object)["skin"].as<std::string>();
                    (*_game->lua_state)["scenes"][scene]["items"][object_id]["skin"] = skin;

                    interactable->setSkin(skin);
                }
                _game->add(interactable);
            }
        }
        else
        {
            float inactivLayerBorder = (*_game->lua_state)["inactivLayerBorder"];
            _game->setInactivLayerBorder(static_cast<int>(inactivLayerBorder));

            // Load from Lua and not from json
            sol::table items = (*_game->lua_state)["scenes"][scene]["items"];
            for (const auto &key_value_pair : items)
            {
                sol::object key = key_value_pair.first;
                sol::object value = key_value_pair.second;

                std::string id = key.as<std::string>();

                if ((*_game->lua_state)["scenes"][scene]["items"][id]["spine"].valid() &&
                    (*_game->lua_state)["scenes"][scene]["items"][id]["x"].valid() &&
                    (*_game->lua_state)["scenes"][scene]["items"][id]["y"].valid())
                {
                    auto interactable = createObject((*_game->lua_state)["scenes"][scene]["items"][id]["spine"], id, (*_game->lua_state)["scenes"][scene]["items"][id]["scale"]);

                    std::string x = (*_game->lua_state)["scenes"][scene]["items"][id]["x"];
                    std::string y = (*_game->lua_state)["scenes"][scene]["items"][id]["y"];
                    bool visible = (*_game->lua_state)["scenes"][scene]["items"][id]["visible"];
                    float layer = (*_game->lua_state)["scenes"][scene]["items"][id]["layer"];
                    std::string animation = (*_game->lua_state)["scenes"][scene]["items"][id]["animation"];
                    bool cross_scene = (*_game->lua_state)["scenes"][scene]["items"][id]["cross_scene"];
                    bool abs_position = (*_game->lua_state)["scenes"][scene]["items"][id]["abs_position"];

                    interactable->setPosition(jngl::Vec2(std::stof(x), std::stof(y)));
                    interactable->setLuaIndex(id);
                    interactable->setVisible(visible);
                    interactable->layer = (int)layer;
                    interactable->cross_scene = cross_scene;
                    interactable->abs_position = abs_position;

                    if (animation == "")
                    {
                        animation = _game->config["spine_default_animation"].as<std::string>();
                    }
                    interactable->playAnimation(0, animation, true, (*_game->lua_state)["pass"]);

                    (*_game->lua_state)["scenes"][scene]["items"][id]["object"] = std::static_pointer_cast<SpineObject>(interactable);

                    if ((*_game->lua_state)["scenes"][scene]["items"][id]["skin"].valid() )
                    {
                        std::string skin = (*_game->lua_state)["scenes"][scene]["items"][id]["skin"];

                        interactable->setSkin(skin);
                    }
                    _game->add(interactable);
                }
            }
        }
    }
}

std::shared_ptr<InteractableObject> Scene::createObject(const std::string &spine_file, std::string id, float scale)
{
    if (auto _game = game.lock())
    {
        return std::make_shared<InteractableObject>(_game, spine_file, id, scale);
    }
    return nullptr;
}


#ifndef NDEBUG
void Scene::writeToFile()
{
    YAML::Emitter emitter1;
    emitter1 << YAML::DoubleQuoted << YAML::LowerNull << json;
    emitter1.SetIndent(4);
    emitter1.SetMapFormat(YAML::Block);
    std::ofstream fout("./../data-src/scenes/" + fileName + ".json");
    jngl::debugLn("Rewrite ./../data-src/scenes/" + fileName + ".json");
    fout << emitter1.c_str();
}


void Scene::updateObjectPosition(const std::string &id, jngl::Vec2 position)
{
    auto objects = json["items"].size();
    for (std::size_t i = 0; i < objects; i++)
    {
        if (json["items"][i]["spine"].as<std::string>() == id)
        {
            json["items"][i]["x"] = std::to_string(position.x);
            json["items"][i]["y"] = std::to_string(position.y);
        }
    }
}
#endif
