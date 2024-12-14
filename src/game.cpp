#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include <spine/spine.h>
#include "pointer.hpp"
#include "interactable_object.hpp"
#include "scene_fade.hpp"

#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
#include "FileWatch.hpp"
#include <filesystem>
#endif

using jngl::Vec2;
using namespace std::string_literals;

Game::Game(const YAML::Node &config) : config(config),
									   cameraPosition(jngl::Vec2(0, 0)),
									   targetCameraPosition(jngl::Vec2(0, 0))
#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
									   ,
									   gifGameFrame(0),
									   gifTime(0.0)
#endif
{

	auto screensize = jngl::getScreenSize();
	auto zoomx = this->config["screenSize"]["x"].as<int>() / screensize.x;
	auto zoomy = this->config["screenSize"]["y"].as<int>() / screensize.y;
	cameraZoom = 1.0 / std::max(zoomx, zoomy);

#ifndef NDEBUG
	debug_info.setText(debug_text);

	debug_info.setPos(jngl::Vec2(-screensize.x / 2, -screensize.y / 2));
#endif

	bool language_supportet = false;
	language = jngl::getPreferredLanguage();

	YAML::Node languages = this->config["supportedLanguages"];
	for (auto supported_language : languages)
	{
		if (language == supported_language.as<std::string>())
		{
			language_supportet = true;
			break;
		}
	}

	if (!language_supportet)
	{
		if (languages.size() >= 1)
		{
			language = languages[0].as<std::string>();
		}
		else
		{
			language = "en";
		}
	}

#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
	gifAnimation = std::make_shared<GifAnim>();
	gifWriter = std::make_shared<GifWriter>();
	gifBuffer = std::make_unique<uint8_t[]>((jngl::getWindowWidth() / GIF_DOWNSCALE_FACTOR * (jngl::getWindowHeight() / GIF_DOWNSCALE_FACTOR)) * 4);
#endif

	// open some common libraries
	lua_state = std::make_shared<sol::state>();
	lua_state->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math);

#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
#ifdef _WIN32
#define TYPE std::wstring
#else
#define TYPE std::string
#endif
	const static filewatch::FileWatch<TYPE> watch(
#ifdef _WIN32
		L"."s,
#else
		"."s,
#endif
		[this](const TYPE &path [[maybe_unused]], const filewatch::Event change_type)
		{
			switch (change_type)
			{
			case filewatch::Event::modified:
			case filewatch::Event::added:
#ifdef _WIN32
				if (path.find(L"webp") != TYPE::npos)
#elif __unix__
				if (path.find("webp") != TYPE::npos)
#else // Mac has no file extension here
				if (true)
#endif
				{
					reload = true;
				}
				break;
			default:
				break;
			}
		});
#endif
}

void Game::init()
{
	configToLua();
	setupLuaFunctions();
	dialogManager = std::make_shared<DialogManager>(shared_from_this());
	if (config["auto_load_savegame"].as<bool>(true))
	{
		loadLuaState();
	}else{
		loadLuaState(std::nullopt);
	}
}

void Game::configToLua()
{
	(*lua_state)["config"] = (*lua_state).create_table();
	(*lua_state)["config"]["name"] = config["name"].as<std::string>();
    (*lua_state)["config"]["screenSize"] = (*lua_state).create_table();
    (*lua_state)["config"]["screenSize"]["x"] = config["screenSize"]["x"].as<int>();
    (*lua_state)["config"]["screenSize"]["y"] = config["screenSize"]["y"].as<int>();
    (*lua_state)["config"]["minAspectRatio"] = (*lua_state).create_table();
    (*lua_state)["config"]["minAspectRatio"]["x"] = config["minAspectRatio"]["x"].as<int>();
    (*lua_state)["config"]["minAspectRatio"]["y"] = config["minAspectRatio"]["y"].as<int>();
    (*lua_state)["config"]["maxAspectRatio"] = (*lua_state).create_table();
    (*lua_state)["config"]["maxAspectRatio"]["x"] = config["maxAspectRatio"]["x"].as<int>();
    (*lua_state)["config"]["maxAspectRatio"]["y"] = config["maxAspectRatio"]["y"].as<int>();
    (*lua_state)["config"]["default_font"] = config["default_font"].as<std::string>();
    (*lua_state)["config"]["default_font_color"] = config["default_font_color"].as<std::string>();
    (*lua_state)["config"]["default_font_selected_color"] = config["default_font_selected_color"].as<std::string>();
    (*lua_state)["config"]["default_font_not_selected_color"] = config["default_font_not_selected_color"].as<std::string>();
    (*lua_state)["config"]["player"] = config["player"].as<std::string>("");
    (*lua_state)["config"]["pointer"] = config["pointer"].as<std::string>();
    (*lua_state)["config"]["dialog"] = config["dialog"].as<std::string>();
    (*lua_state)["config"]["antiAliasing"] = config["antiAliasing"].as<bool>();
    (*lua_state)["config"]["icon"] = config["icon"].as<std::string>();
    (*lua_state)["config"]["start_scene"] = config["start_scene"].as<std::string>();
    (*lua_state)["config"]["double_click_time"] = config["double_click_time"].as<float>();
    (*lua_state)["config"]["max_click_distance"] = config["max_click_distance"].as<int>();
#ifndef NDEBUG
    (*lua_state)["config"]["debug_grap_distance"] = config["debug_grap_distance"].as<float>();
#endif
    (*lua_state)["config"]["gamepad_speed_multiplier"] = config["gamepad_speed_multiplier"].as<float>();
    (*lua_state)["config"]["inventory_default_skin"] = config["inventory_default_skin"].as<std::string>();
    (*lua_state)["config"]["player_default_skin"] = config["player_default_skin"].as<std::string>();
    (*lua_state)["config"]["player_side_skin"] = config["player_side_skin"].as<std::string>();
    (*lua_state)["config"]["player_front_skin"] = config["player_front_skin"].as<std::string>();
    (*lua_state)["config"]["player_up_skin"] = config["player_up_skin"].as<std::string>();
    (*lua_state)["config"]["player_walk_animation"] = config["player_walk_animation"].as<std::string>();
    (*lua_state)["config"]["player_beam_animation"] = config["player_beam_animation"].as<std::string>();
    (*lua_state)["config"]["player_idle_animation"] = config["player_idle_animation"].as<std::string>();
    (*lua_state)["config"]["player_start_animation"] = config["player_start_animation"].as<std::string>();
    (*lua_state)["config"]["spine_default_animation"] = config["spine_default_animation"].as<std::string>();
    (*lua_state)["config"]["pointer_idle_animation"] = config["pointer_idle_animation"].as<std::string>();
    (*lua_state)["config"]["pointer_over_animation"] = config["pointer_over_animation"].as<std::string>();
    (*lua_state)["config"]["background_default_animation"] = config["background_default_animation"].as<std::string>();
    (*lua_state)["config"]["pointer_max_speed"] = config["pointer_max_speed"].as<float>();
    (*lua_state)["config"]["player_max_speed"] = config["player_max_speed"].as<float>();
    (*lua_state)["config"]["player_start_position"] = (*lua_state).create_table();
    (*lua_state)["config"]["player_start_position"]["x"] = config["player_start_position"]["x"].as<int>();
    (*lua_state)["config"]["player_start_position"]["y"] = config["player_start_position"]["y"].as<int>();
    (*lua_state)["config"]["border"] = (*lua_state).create_table();
    (*lua_state)["config"]["border"]["x"] = config["border"]["x"].as<int>();
    (*lua_state)["config"]["border"]["y"] = config["border"]["y"].as<int>();
	(*lua_state)["config"]["supportedLanguages"] = sol::as_table(config["supportedLanguages"].as<std::vector<std::string>>());
}

void Game::loadSceneWithFade(const std::string &level)
{
	if(enable_fade){
		jngl::setWork<SceneFade>(jngl::getWork(), [this, level]() {
			loadScene(level);
		});
	}else{
		loadScene(level);
	}
#ifndef NDEBUG
	// Run tests to get a savegame at the start of each scene
	auto old_savegame = jngl::readConfig(level);
	if(old_savegame.empty())
	{
		saveLuaState(level);
	}
#endif
}

void Game::loadScene(const std::string& level)
{
	std::string old_scene;
	if (currentScene)
	{
		old_scene = currentScene->getSceneName();
	}
	dialogManager->cancelDialog();

	// Clear the level if there is already a level loaded
	for (auto it = gameObjects.rbegin(); it != gameObjects.rend();)
	{
		remove(*it);
		std::advance(it, 1);
	}
	player = nullptr;

	auto newScene = std::make_shared<Scene>(level, shared_from_this());
	if (!newScene->background)
	{
		jngl::error("There is no scene with the name: " + level);
		newScene = std::make_shared<Scene>(old_scene, shared_from_this());
	}

	currentScene = newScene;
	currentScene->background->step();
	currentScene->playMusic();

	// Pointer should be last in gameObjects so it's on top
	if (pointer == nullptr)
	{
		pointer = std::make_shared<Pointer>(shared_from_this(), (*lua_state)["config"]["pointer"]);
		pointer->setCrossScene(true);
		pointer->setPosition(Vec2(0, 0));
		add(pointer);
	}

	if (player)
	{
		auto position = currentScene->background->getPoint(old_scene);
		if (position)
		{
			player->setPosition(position.value());
		}
		player->stop_walking();
		setCameraPositionImmediately(player->calcCamPos());
	}

	runAction(level, newScene->background);
}

Game::~Game()
{
	saveLuaState();
	reset();
}

void Game::reset()
{
	gameObjects.clear();
	needToAdd.clear();
	needToRemove.clear();
	lua_state = {};
	currentScene = nullptr;
	player = nullptr;
	pointer = nullptr;
}

void Game::step()
{
	addObjects();
	stepCamera();

	pointer->step();

	for (auto it = gameObjects.rbegin(); it != gameObjects.rend();)
	{
		if ((*it) == pointer)
		{
			std::advance(it, 1);
			continue;
		}
		if ((*it) == nullptr || (*it)->step())
		{
			remove(*it);
		}
		std::advance(it, 1);
	}

	dialogManager->step();

	// Sort game objects depending on the y position for
	sort(gameObjects.begin(), gameObjects.end(), [](const auto &lhs, const auto &rhs)
		 { return lhs->getZ() < rhs->getZ(); });

#ifndef NDEBUG
	debugStep();
#endif
	pointer->resetHandledFlags();
	removeObjects();
}

#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
static std::string currentDateTime()
{
	const time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d-%M-%S", &tstruct);

	return buf;
}
#endif

#ifndef NDEBUG
void Game::debugStep()
{
	// Reload Scene
	if (jngl::keyPressed("r") || reload)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		auto dialogFilePath = (*lua_state)["config"]["dialog"];
		getDialogManager()->loadDialogsFromFile(dialogFilePath, false);
		loadScene(currentScene->getSceneName());
		reload = false;
	}

	if (jngl::keyPressed(jngl::key::F10))
	{
		enableDebugDraw = !enableDebugDraw;
	}
	if (jngl::keyPressed('m'))
	{
		if (jngl::getVolume() > 0.0)
	{
		jngl::setVolume(0);
		}
		else
		{
			jngl::setVolume(1.0);
		}
	}
	if (jngl::keyPressed('z'))
	{
		enablezMapDebugDraw = !enablezMapDebugDraw;
	}
	if (jngl::keyPressed(jngl::key::Tab))
	{
		editMode = !editMode;
	}
	if (editMode && jngl::keyPressed("s"))
	{
		currentScene->writeToFile();
	}

	if (jngl::keyPressed("x"))
	{
		show_debug_info = !show_debug_info;
	}

	// Quick Save
	if (jngl::keyPressed("c"))
	{
		saveLuaState();
	}

	// Quick Load
	if (jngl::keyPressed("v"))
	{
		reset();
		lua_state = std::make_shared<sol::state>();
		lua_state->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math);
		init();
	}

	// Restart Game
	if (jngl::keyPressed("l"))
	{
		jngl::writeConfig("savegame", "");
		reset();
		lua_state = std::make_shared<sol::state>();
		lua_state->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math);
		init();
	}

	if (!room_select_mode)
		{
		// Save with Control + 1-9 and Load with 1-9
		for (const auto &number : {"1", "2", "3", "4", "5", "6", "7", "8", "9"})
		{
			if (jngl::keyDown(jngl::key::ControlL) && jngl::keyPressed(number))
			{
				jngl::debugLn("Save to Save " + std::string(number));
				saveLuaState("savegame" + std::string(number));
			}
			else if (jngl::keyPressed(number))
			{
				reset();
				lua_state = std::make_shared<sol::state>();
				lua_state->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math);

				configToLua();
				setupLuaFunctions();
				loadLuaState("savegame" + std::string(number));
				dialogManager = std::make_shared<DialogManager>(shared_from_this());
			}
		}
	}

#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
	for (const auto &number : {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"})
	{
		int x = static_cast<int>(number[0]) - static_cast<int>('0');
		if (room_select_mode && jngl::keyPressed(number) && tens.has_value())
		{
			const std::string path = jngl::internal::getConfigPath();
			const int target = tens.value() * 10 + x;

			int i = 0;
			for (const auto & entry [[maybe_unused]] : std::filesystem::directory_iterator(path))
			{
				i++;
			}
			if (target < i) {
				reset();
				lua_state = std::make_shared<sol::state>();
				lua_state->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math);

				configToLua();
				setupLuaFunctions();

				i = 0;
				for (const auto & entry : std::filesystem::directory_iterator(path))
				{
					if (i == target) {
#ifdef _WIN32
						std::wstring wide = std::wstring(entry.path().filename());
						std::string str( wide.begin(), wide.end() );
						loadLuaState(str);
#else
						loadLuaState(std::string(entry.path().filename()));
#endif
						dialogManager = std::make_shared<DialogManager>(shared_from_this());
						const std::string dialogFilePath = (*lua_state)["config"]["dialog"];
						dialogManager->loadDialogsFromFile(dialogFilePath, false);
#ifdef _WIN32
						runAction(std::string(str), std::static_pointer_cast<SpineObject>(currentScene->background));
#else
						runAction(std::string(entry.path().filename()), std::static_pointer_cast<SpineObject>(currentScene->background));
#endif
					}
					i++;
				}
				room_select_mode = false;
				tens.reset();
				debug_info.setText(debug_text);
			}else {
				room_select_mode = false;
				tens.reset();
				debug_info.setText(debug_text);
			}
		}
		else if (room_select_mode && jngl::keyPressed(number) && !tens.has_value())
		{
			tens = x;
		}
	}

	// Jump to a room
	if (jngl::keyPressed("j"))
	{
		int i = 0;
		std::string files;

		const std::string path = jngl::internal::getConfigPath();
		for (const auto & entry : std::filesystem::directory_iterator(path))
		{
#ifdef _WIN32
			std::wstring wide = std::wstring(entry.path().filename());
			std::string str( wide.begin(), wide.end() );
			files += std::to_string(i) + " " + str + "\n";
#else
			files += std::to_string(i) + " " + std::string(entry.path().filename()) + "\n";
#endif
			i++;
		}

		debug_info.setText(files);
		room_select_mode = true;
	}

	if (recordingGif)
	{
		if (gifGameFrame % GIF_FRAME_SKIP == 0)
		{
			auto pixels = jngl::readPixels();
			const int w = jngl::getWindowWidth() / GIF_DOWNSCALE_FACTOR;
			const int h = jngl::getWindowHeight() / GIF_DOWNSCALE_FACTOR;

			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					gifBuffer[((w * y) + x) * 4] = static_cast<uint8_t>(pixels[((w * GIF_DOWNSCALE_FACTOR * (h * GIF_DOWNSCALE_FACTOR - 1 * GIF_DOWNSCALE_FACTOR - y * GIF_DOWNSCALE_FACTOR)) + x * GIF_DOWNSCALE_FACTOR) * 3] * 255);
					gifBuffer[((w * y) + x) * 4 + 1] = static_cast<uint8_t>(pixels[((w * GIF_DOWNSCALE_FACTOR * (h * GIF_DOWNSCALE_FACTOR - 1 * GIF_DOWNSCALE_FACTOR - y * GIF_DOWNSCALE_FACTOR)) + x * GIF_DOWNSCALE_FACTOR) * 3 + 1] * 255);
					gifBuffer[((w * y) + x) * 4 + 2] = static_cast<uint8_t>(pixels[((w * GIF_DOWNSCALE_FACTOR * (h * GIF_DOWNSCALE_FACTOR - 1 * GIF_DOWNSCALE_FACTOR - y * GIF_DOWNSCALE_FACTOR)) + x * GIF_DOWNSCALE_FACTOR) * 3 + 2] * 255);
					gifBuffer[((w * y) + x) * 4 + 3] = 255;
				}
			}

			auto currentTime = jngl::getTime();
			auto timeDiff = currentTime - gifTime;

			gifAnimation->GifWriteFrame(gifWriter.get(),
										gifBuffer.get(),
										jngl::getWindowWidth() / GIF_DOWNSCALE_FACTOR,
										jngl::getWindowHeight() / GIF_DOWNSCALE_FACTOR,
										static_cast<uint32_t>(timeDiff),
										8,
										false,
										nullptr);

			gifTime = currentTime;
		}
		gifGameFrame++;
	}

	if (jngl::keyPressed(jngl::key::F12))
	{
		if (!recordingGif)
		{
			// start recording
			recordingGif = true;
			show_debug_info = false;

			gifGameFrame = 0;
			gifTime = jngl::getTime();

			std::string filename = "./../";
			filename += currentDateTime() + ".gif";
			jngl::debug("start recording ");
			jngl::debugLn(filename);

			gifAnimation->GifBegin(gifWriter.get(),
								   filename.c_str(),
								   jngl::getWindowWidth() / GIF_DOWNSCALE_FACTOR,
								   jngl::getWindowHeight() / GIF_DOWNSCALE_FACTOR,
								   100, // providing 0 ruins the loop count argument ¯\_(ツ)_/¯
								   0,
								   8,
								   false);
		}
		else
		{
			// stop recording
			recordingGif = false;
			gifAnimation->GifEnd(gifWriter.get());
			jngl::debugLn("stop recording");
		}
	}
#endif
}
#endif

void Game::draw() const
{
	jngl::pushMatrix();
	jngl::setBackgroundColor(jngl::Color(0, 0, 0));
	applyCamera();
	jngl::setColor(30, 200, 30, 255);

	for (auto &obj : gameObjects)
	{
		if ((obj) == pointer)
		{
			continue;
		}
		if (obj->getVisible())
		{
			obj->draw();
		}
	}
	jngl::popMatrix();

	jngl::pushMatrix();
	dialogManager->draw();
	// Der Pointer wird doppelt gedrawed, damit der immer vorne ist.
	pointer->draw();

#ifndef NDEBUG
	if (show_debug_info)
	{
		jngl::setFontColor(0, 0, 0, 255);
		debug_info.draw();
	}
#endif

	jngl::popMatrix();
}

void Game::applyCamera() const
{
	jngl::scale(cameraZoom);
	jngl::translate(-1 * cameraPosition / cameraZoom);
}

double Game::getCameraZoom() const
{
	return cameraZoom;
}

Vec2 Game::getCameraSpeed() const
{
	return targetCameraPosition - cameraPosition;
}

Vec2 Game::getCameraPosition() const
{
	return cameraPosition;
}

template <typename T>
int sgn(T val)
{
	return (T(0) < val) - (val < T(0));
}

void Game::setCameraPosition(Vec2 position, const double deadzoneFactorX,
							 const double deadzoneFactorY)
{
	if (position.x < currentScene->left_border)
	{
		position.x = currentScene->left_border;
	}
	if (position.x > currentScene->right_border)
	{
		position.x = currentScene->right_border;
	}
	if (position.y < currentScene->top_border)
	{
		position.y = currentScene->top_border;
	}
	if (position.y > currentScene->bottom_border)
	{
		position.y = currentScene->bottom_border;
	}

	cameraDeadzone = position - targetCameraPosition;
	const double x = 160 * deadzoneFactorX;
	const double y = 90 * deadzoneFactorY;
	if (std::abs(cameraDeadzone.x) > x)
	{
		cameraDeadzone.x =
			sgn(cameraDeadzone.x) *
			(x + (std::abs(cameraDeadzone.x) - x) / std::exp(std::abs(cameraDeadzone.x) - x));
	}
	if (std::abs(cameraDeadzone.y) > y)
	{
		cameraDeadzone.y =
			sgn(cameraDeadzone.y) *
			(y + (std::abs(cameraDeadzone.y) - y) / std::exp(std::abs(cameraDeadzone.y) - y));
	}
	targetCameraPosition = position - cameraDeadzone;
}

void Game::setCameraPositionImmediately(Vec2 position)
{
	if (position.x < currentScene->left_border)
	{
		position.x = currentScene->left_border;
	}
	if (position.x > currentScene->right_border)
	{
		position.x = currentScene->right_border;
	}
	if (position.y < currentScene->top_border)
	{
		position.y = currentScene->top_border;
	}
	if (position.y > currentScene->bottom_border)
	{
		position.y = currentScene->bottom_border;
	}
	targetCameraPosition = cameraPosition = position;
}

void Game::stepCamera()
{
	const auto speed = getCameraSpeed();
	cameraPosition += speed / 36.0;
}

void Game::add(const std::shared_ptr<SpineObject> &obj)
{
	needToAdd.emplace_back(obj);
}

void Game::remove(const std::shared_ptr<SpineObject> &object)
{
	needToRemove.push_back(object);
}

void Game::addObjects()
{
	std::copy(needToAdd.begin(), needToAdd.end(), std::back_inserter(gameObjects));
	needToAdd.clear();
}

void Game::removeObjects()
{
	for (const auto &toRemove : needToRemove)
	{
		for (auto it = gameObjects.begin(); it != gameObjects.end(); ++it)
		{
			if (*it == toRemove)
			{
				gameObjects.erase(it);
				break;
			}
		}
	}
	needToRemove.clear();
}

std::shared_ptr<DialogManager> Game::getDialogManager()
{
	return dialogManager;
}

AudioManager *Game::getAudioManager()
{
	return &audioManager;
}

void Game::runAction(const std::string &actionName, std::shared_ptr<SpineObject> thisObject)
{
	if (actionName.empty())
	{
		return;
	}

	std::string errorMessage;
	std::string script;
	lua_state->set("this", thisObject);

	// if the name starts with "dlg:", play the dialog,
	// no need for a separate Lua file
	if (actionName.substr(0, 4) == "dlg:")
	{
		const std::string dialogName = actionName.substr(4);
		script = "PlayDialog(\"" + dialogName + "\", pass)";
		errorMessage = "Failed to play dialog " + dialogName + "!\n";
	}
	else if (actionName.substr(0, 5) == "anim:")
	{
		const std::string animName = actionName.substr(5);
		script = "PlayAnimationOn(\"" + thisObject->getId() + "\", 0, \"" + animName + "\", false, pass)";
		errorMessage = "Failed to play animation " + animName + "!\n";
	}
	// if there is no specific prefix, just load the according Lua file
	else
	{
		const std::string file = "scripts/" + actionName + ".lua";
		errorMessage = "The Lua code of " + file + " has failed to run!\n";
		const std::stringstream scriptstream = jngl::readAsset(file);

		if (!scriptstream)
		{
			jngl::error("Can not load lua script " + file);
			return;
		}
		script = scriptstream.str();
	}

	auto result = lua_state->safe_script(script, sol::script_pass_on_error);

	if (!result.valid())
	{
		const sol::error err = result;
		std::cerr << errorMessage
				  << err.what()
				  << std::endl;
	}
}

void Game::saveLuaState(const std::string &savefile)
{
	if (savefile.empty())
	{
		return;
	}
	// jngl::debugLn("Backup all globals start");
	const sol::table &globals = lua_state->globals();

	const std::string backup = backupLuaTable(globals, "");
	jngl::debugLn("Backup all globals end: \n" + backup);

	jngl::writeConfig(savefile, backup);
}

void Game::loadLuaState(const std::optional<std::string> &savefile)
{
	jngl::debugLn("Load all globals");
	if (savefile) {
		const std::string state = jngl::readConfig(savefile.value());
		auto result = lua_state->safe_script(state, sol::script_pass_on_error);

		if (!result.valid())
		{
			const sol::error err = result;
			std::cerr << "Failed to load savgame " + savefile.value() + " \n"
					<< err.what()
					<< std::endl;
		}
	}

	const std::string dialogFilePath = (*lua_state)["config"]["dialog"];
	if ((*lua_state)["game"].valid() && (*lua_state)["game"]["scene"].valid())
	{
		getDialogManager()->loadDialogsFromFile(dialogFilePath, false);
		const std::string scene = (*lua_state)["game"]["scene"];
		loadScene(scene);
	}
	else
	{
		getDialogManager()->loadDialogsFromFile(dialogFilePath, true);
		const std::string startscene = (*lua_state)["config"]["start_scene"];
		loadScene(startscene);
	}
	// TODO Error handling
	jngl::debugLn("Loaded all globals");
}

const std::string Game::cleanLuaString(std::string variable)
{
	// The following strings denote other tokens:

	//  +     -     *     /     %     ^     #
	//  ==    ~=    <=    >=    <     >     =
	//  (     )     {     }     [     ]
	//  ;     :     ,     .     ..    ...
	for (const char *const &invalidChar : {"+", "-", "*", "/", "%", "^", "#", "<", ">", "=", "(", ")", "{", "}", "[", "]", ";", ":", ",", "."})
	{
		while (variable.find(invalidChar) != std::string::npos)
		{
			variable.replace(variable.find(invalidChar), 1, "_");
		}
	}

	return variable;
}

std::string Game::backupLuaTable(const sol::table table, const std::string &parent)
{
	std::string result;
	for (const auto &key_value_pair : table)
	{
		const sol::object key = key_value_pair.first;
		const sol::object value = key_value_pair.second;

		std::string k;
		if (key.get_type() == sol::type::string)
		{
			k = key.as<std::string>();
		}
		else if (key.get_type() == sol::type::number)
		{
			k = std::to_string(key.as<int>());
		}

		if (!parent.empty())
		{
			if (key.get_type() == sol::type::string)
			{
				k = "[\"" + k + "\"]";
			}
			else if (key.get_type() == sol::type::number)
			{
				k = "[" + k + "]";
			}
		}

		if (k != "_entry_node" &&
			k != "_VERSION" &&
			k.substr(0, 4) != "sol." &&
			k != "_G" &&
			k != "base" &&
			k != "package" &&
			k != "math" &&
			k != "string" &&
			k != "searches")
		{
			std::string v;
			switch (value.get_type())
			{
				// All this types are not saved in our savegame file.
			case sol::type::none:
			case sol::type::lua_nil:
			case sol::type::thread:
			case sol::type::function:
			case sol::type::userdata:
			case sol::type::poly:
			case sol::type::lightuserdata:
				break;

			case sol::type::string:
				v = value.as<std::string>();
				result += parent + k + " = \"" + v + "\"\n";
				break;
			case sol::type::number:
				if (value.is<int>())
				{
					v = std::to_string(value.as<int>());
				}else{
				v = std::to_string(value.as<double>());
				}
				result += parent + k + " = " + v + "\n";
				break;
			case sol::type::boolean:

				if (value.as<bool>())
				{
					v = "true";
				}
				else
				{
					v = "false";
				}

				result += parent + k + " = " + v + "\n";
				break;
			case sol::type::table:
				result += parent + k + " = {}\n";
				result += backupLuaTable(value.as<sol::table>(), parent + k);
				break;
			}
		}
	}

	return result;
}

const std::shared_ptr<SpineObject> Game::getObjectById(const std::string &objectId)
{
	if (objectId == "Player" || objectId == "player")
	{
		return (*this->lua_state)["scenes"]["cross_scene"]["items"]["player"]["object"];
	}
	if (objectId == "Background")
	{
		return currentScene->background;
	}

	std::shared_ptr<SpineObject> obj = nullptr;
	if ((*this->lua_state)[objectId].valid())
	{
		obj = (*this->lua_state)[objectId];
	}
	else if ((*this->lua_state)["inventory_items"][objectId].valid())
	{
		obj = (*this->lua_state)["inventory_items"][objectId]["object"];
	}
	else
	{
		std::string scene = (*this->lua_state)["game"]["scene"];
		if ((*this->lua_state)["scenes"][scene]["items"][objectId].valid())
		{
			obj = (*this->lua_state)["scenes"][scene]["items"][objectId]["object"];
		}
	}
	if (obj == nullptr)
	{
		if ((*this->lua_state)["scenes"]["cross_scene"]["items"][objectId].valid())
		{
			obj = (*this->lua_state)["scenes"]["cross_scene"]["items"][objectId]["object"];
		}
	}
	return obj;
}

const std::string Game::getLuaPath(std::string objectId)
{
	if (objectId == "Player" || objectId == "player")
	{
		return R"(scenes["cross_scene"]["items"]["player"])";
	}

	std::string scene = (*this->lua_state)["game"]["scene"];
	if (objectId == "Background") {
		return "scenes[\"" + scene + R"("]["background"])";
	}

        if ((*this->lua_state)["inventory_items"][objectId].valid())
	{
		return "inventory_items[\"" + objectId + "\"]";
	}

	if ((*this->lua_state)["scenes"][scene]["items"][objectId].valid())
	{
		return "scenes[\"" + scene + R"("]["items"][")" + objectId + "\"]";
	}

	if ((*this->lua_state)["scenes"]["cross_scene"]["items"][objectId].valid())
	{
		return R"(scenes["cross_scene"]["items"][")" + objectId + "\"]";
	}
	return "";
}

#if (!defined(NDEBUG) && !defined(ANDROID) && (!defined(TARGET_OS_IOS) || TARGET_OS_IOS == 0) && !defined(EMSCRIPTEN))
void Game::onFileDrop(const std::filesystem::path& path)
{
	std::string spine_file = path.stem();

    auto atlas = spAtlas_createFromFile((spine_file + "/" + spine_file + ".atlas").c_str(), nullptr);
    assert(atlas);
    spSkeletonJson *json = spSkeletonJson_create(atlas);
	auto skeletonData = spSkeletonJson_readSkeletonDataFile(json, (spine_file + "/" + spine_file + ".json").c_str());

	if (!skeletonData)
	{
		jngl::error("Error loading " + spine_file + " Spine project. Make sure it is saved in data-src and prepare_assets is running.");
		return;
	}
	currentScene->addToFile(spine_file);
	currentScene->writeToFile();
}
#endif
