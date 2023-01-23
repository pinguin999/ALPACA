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

#if (!defined(NDEBUG) && !defined(ANDROID) && !defined(EMSCRIPTEN))
#include "FileWatch.hpp"
#include <filesystem>
#endif

using jngl::Vec2;
using namespace std::string_literals;

Game::Game(YAML::Node config) : config(config)
{
	setCameraPositionImmediately(getCameraOrigin());

	auto screensize = jngl::getScreenSize();
	auto zoomx = this->config["screenSize"]["x"].as<int>() / screensize.x;
	auto zoomy = this->config["screenSize"]["y"].as<int>() / screensize.y;
	cameraZoom = 1.0 / std::max(zoomx, zoomy);

	language = jngl::getPreferredLanguage();

#if (!defined(NDEBUG) && !defined(ANDROID) && !defined(EMSCRIPTEN))
    gifAnimation = new GifAnim();
    gifWriter = new GifWriter();
	gifBuffer = new uint8_t[(jngl::getWindowWidth() / GIF_DOWNSCALE_FACTOR * (jngl::getWindowHeight() / GIF_DOWNSCALE_FACTOR)) * 4];
#endif

	// open some common libraries
	lua_state = std::make_shared<sol::state>();
	lua_state->open_libraries(sol::lib::base, sol::lib::package);

#if (!defined(NDEBUG) && !defined(ANDROID) && !defined(EMSCRIPTEN))
#ifdef _WIN32
#define TYPE std::wstring
#else
#define TYPE std::string
#endif
	static filewatch::FileWatch<TYPE> watch(
#ifdef _WIN32
		L"."s,
#else
		"."s,
#endif
		[this](const TYPE &path, const filewatch::Event change_type)
		{
			switch (change_type)
			{
			case filewatch::Event::modified:
			case filewatch::Event::added:
#ifdef _WIN32
				if (path.find(L"webp") != TYPE::npos)
#elif __unix__
				if (path.find("webp") != TYPE::npos)
#else  // Mac has no file extension here
				if(true)
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
	dialogManager = std::make_shared<DialogManager>(shared_from_this());
	setupLuaFunctions();
	loadLuaState();
}

// TODO in LoadScene umbenennen
void Game::loadLevel(const std::string &level)
{
	dialogManager->cancelDialog();

	// Clear the level if there is already a level loaded
	for (auto it = gameObjects.begin(); it != gameObjects.end();)
	{
		if ((*it)->cross_scene)
		{
			++it;
		}
		else
		{
			(*it)->setDeleted();
			++it;
		}
	}

	currentScene = std::make_shared<Scene>(level, shared_from_this());
	currentScene->playMusic();

	// Pointer should be last in gameObjects so it's on top
	if (pointer == nullptr)
	{
		pointer = std::make_shared<Pointer>(shared_from_this(), config["pointer"].as<std::string>());
		pointer->cross_scene = true;
		pointer->setPosition(Vec2(0, 0));
		add(pointer);
	}
}

Game::~Game()
{
	saveLuaState();
	gameObjects.clear();
	needToAdd.clear();
	needToRemove.clear();
#if (!defined(NDEBUG) && !defined(ANDROID) && !defined(EMSCRIPTEN))
	free(gifBuffer);
#endif
}

void Game::step()
{
	addObjects();
	stepCamera();

	pointer->step();

	for (auto it = gameObjects.rbegin(); it != gameObjects.rend();)
	{
		if((*it) == pointer)
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

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

#ifndef NDEBUG
void Game::debugStep()
{
#if (!defined(NDEBUG) && !defined(ANDROID) && !defined(EMSCRIPTEN))
	if (jngl::keyPressed(jngl::key::F12))
	{
		if(!recordingGif)
		{
			// start recording
			recordingGif = true;

			gifFrame = 0;
			gifGameFrame = 0;
			gifTime = jngl::getTime();

			std::string filename = "";
			filename += currentDateTime() + ".gif";
			jngl::debug("start recording ");
			jngl::debugLn(filename);

			gifAnimation->GifBegin(gifWriter,
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
			gifAnimation->GifEnd(gifWriter);
			jngl::debugLn("stop recording");
		}
	}
#endif

	// Reload Scene
	if (jngl::keyPressed("r") || reload)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		loadLevel(currentScene->getSceneName());
		reload = false;
	}

	if (jngl::keyPressed(jngl::key::F10))
	{
		enableDebugDraw = !enableDebugDraw;
	}
	if (jngl::keyPressed(jngl::key::Tab))
	{
		editMode = !editMode;
	}
	if (editMode && jngl::keyPressed("S"))
	{
		currentScene->writeToFile();
	}

	// Quick Save
	if (jngl::keyPressed("c"))
	{
		saveLuaState();
	}

	// Quick Load
	if (jngl::keyPressed("v"))
	{
		loadLuaState();
	}

	// End all Animations, I'm not sure jet where to put it.
	if (jngl::keyPressed(jngl::key::Space))
	{
		for (const auto& obj : gameObjects)
		{
			obj->skeleton->endAnimation(0);
		}
	}

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
			loadLuaState("savegame" + std::string(number));
		}
	}

	// if(jngl::keyPressed(jngl::key::Space)) {
	//     dialogManager.showTypewriterAnimation("Freitag, 13. September 2080");
	// }
	// if(jngl::keyPressed(jngl::key::Delete)) {
	//     std::list<std::string> choises;
	//     choises.push_front("Hallo Carsten");
	//     choises.push_front("Hallo Kolja");
	//     choises.push_front("Willkommen zum Hobby Spieleprogrammierer Podcast!");
	//     dialogManager.showChoices(choises);

	//    // dialogManager.showNarratorText("Wir schreiben das Jahr 2080.\nDie Menschheit ist auf der Suche nach außerirdischem Leben Dose");
	//}

	// if(jngl::keyPressed(jngl::key::BackSpace)) {
	//     dialogManager.showCharacterText("Wir schreiben das Jahr 2080.\nDie Menschheit ist auf der Suche nach außerirdischem Leben", jngl::Vec2(0,0));
	// }

#if (!defined(NDEBUG) && !defined(ANDROID) && !defined(EMSCRIPTEN))
	if(recordingGif)
	{
		if(gifGameFrame % GIF_FRAME_SKIP == 0)
		{

			auto pixels = jngl::readPixels();
			int w = jngl::getWindowWidth() / GIF_DOWNSCALE_FACTOR;
			int h = jngl::getWindowHeight() / GIF_DOWNSCALE_FACTOR;

			for(int y = 0; y < h; y++)
			{
				for(int x = 0; x < w; x++)
				{
					gifBuffer[((w * y) + x) * 4]     = (uint8_t)(pixels[((w * GIF_DOWNSCALE_FACTOR * (h * GIF_DOWNSCALE_FACTOR - 1 * GIF_DOWNSCALE_FACTOR - y * GIF_DOWNSCALE_FACTOR)) + x * GIF_DOWNSCALE_FACTOR) * 3] * 255);
					gifBuffer[((w * y) + x) * 4 + 1] = (uint8_t)(pixels[((w * GIF_DOWNSCALE_FACTOR * (h * GIF_DOWNSCALE_FACTOR - 1 * GIF_DOWNSCALE_FACTOR - y * GIF_DOWNSCALE_FACTOR)) + x * GIF_DOWNSCALE_FACTOR) * 3 + 1] * 255);
					gifBuffer[((w * y) + x) * 4 + 2] = (uint8_t)(pixels[((w * GIF_DOWNSCALE_FACTOR * (h * GIF_DOWNSCALE_FACTOR - 1 * GIF_DOWNSCALE_FACTOR - y * GIF_DOWNSCALE_FACTOR)) + x * GIF_DOWNSCALE_FACTOR) * 3 + 2] * 255);
					gifBuffer[((w * y) + x) * 4 + 3] = 255;
				}
			}

			auto currentTime = jngl::getTime();
			auto timeDiff = currentTime - gifTime;

			gifAnimation->GifWriteFrame(gifWriter,
									gifBuffer,
									jngl::getWindowWidth() / GIF_DOWNSCALE_FACTOR,
									jngl::getWindowHeight() / GIF_DOWNSCALE_FACTOR,
									(uint32_t)timeDiff,
									8,
									false,
									nullptr);

			gifFrame++;
			gifTime = currentTime;
		}
		gifGameFrame++;
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

	for (auto& obj : gameObjects)
	{
		if(obj->getVisible())
			obj->draw();
	}

	dialogManager->draw();
	// Der Pointer wird doppelt gedrawed, damit der immer vorne ist.
	pointer->draw();
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

Vec2 Game::getCameraOrigin() const
{
	return Vec2(0, 0);
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
	targetCameraPosition = cameraPosition = position;
}

void Game::stepCamera()
{
	const auto speed = getCameraSpeed();
	cameraPosition += speed / 36.0;
}

void Game::add(std::shared_ptr<SpineObject> obj) {
	needToAdd.emplace_back(obj);
}

void Game::remove(std::shared_ptr<SpineObject> object) {
	needToRemove.push_back(object);
}

void Game::addObjects() {
	std::copy(needToAdd.begin(), needToAdd.end(), std::back_inserter(gameObjects));
	needToAdd.clear();
}

void Game::removeObjects() {
	for (const auto& toRemove : needToRemove) {
		for (auto it = gameObjects.begin(); it != gameObjects.end(); ++it) {
			if (*it == toRemove) {
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

void Game::runAction(std::string actionName, std::shared_ptr<SpineObject> thisObject)
{
	if (actionName == "")
        return;

	std::string errorMessage;
	std::string script;
	lua_state->set("this", thisObject);

	// if the name starts with "dlg:", play the dialog,
	// no need for a separate Lua file
	if(actionName.substr(0, 4) == "dlg:")
	{
		std::string dialogName = actionName.substr(4);
		script = "PlayDialog(\"" + dialogName + "\", pass)";
		errorMessage = "Failed to play dialog " + dialogName + "!\n";
	}
	// if there is no specific prefix, just load the according Lua file
	else
	{
		std::string file = "scripts/" + actionName + ".lua";
		errorMessage = "The LUA code of " + file + " has failed to run!\n";
		std::stringstream scriptstream = jngl::readAsset(file);

		if (!scriptstream)
		{
			jngl::debugLn("Can not load lua script " + file);
			return;
		}
		script = scriptstream.str();
	}

    auto result = lua_state->safe_script(script, sol::script_pass_on_error);

	if (!result.valid()) {
		sol::error err = result;
		std::cerr << errorMessage
		          << err.what()
		          << std::endl;
	}
}

void Game::saveLuaState(std::string savefile)
{
	// jngl::debugLn("Backup all globals start");
	const sol::table &globals = lua_state->globals();

	std::string backup = backupLuaTable(globals, "");
	jngl::debugLn("Backup all globals end: \n" + backup);

	jngl::writeConfig(savefile, backup);
}

void Game::loadLuaState(std::string savefile)
{
	jngl::debugLn("Load all globals");
	std::string state = jngl::readConfig(savefile);
	auto result = lua_state->safe_script(state, sol::script_pass_on_error);

	if (!result.valid())
	{
		sol::error err = result;
		std::cerr << "Failed to load savgame " + savefile + " \n"
				  << err.what()
				  << std::endl;
	}

	std::string dialogFilePath = config["dialog"].as<std::string>();

	if ((*lua_state)["game"].valid() && (*lua_state)["game"]["scene"].valid())
	{
		std::string scene = (*lua_state)["game"]["scene"];
		loadLevel(scene);
	}
	else
	{
		std::string startscene = config["start_scene"].as<std::string>();
		loadLevel(startscene);
	}
	getDialogManager()->loadDialogsFromFile(dialogFilePath);
	// TODO Error handling
	jngl::debugLn("Loaded all globals");
}

std::string Game::cleanLuaString(std::string variable)
{
	// The following strings denote other tokens:

	//  +     -     *     /     %     ^     #
	//  ==    ~=    <=    >=    <     >     =
	//  (     )     {     }     [     ]
	//  ;     :     ,     .     ..    ...
	for (const char* const& invalidChar : {"+", "-", "*", "/", "%", "^", "#", "<", ">", "=", "(", ")", "{", "}", "[", "]", ";", ":", ",", "."})
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
	std::string result = "";
	for (const auto &key_value_pair : table)
	{
		sol::object key = key_value_pair.first;
		sol::object value = key_value_pair.second;

		std::string k = "";
		if (key.get_type() == sol::type::string)
		{
			k = key.as<std::string>();
		}
		else if (key.get_type() == sol::type::number)
		{
			k = std::to_string(key.as<int>());
		}

		if (k != "_entry_node" && k != "_VERSION" && k.substr(0, 4) != "sol." && k != "_G" && k != "base" && k != "package" && k != "searches")
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
				v = std::to_string(value.as<double>());
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
				result += backupLuaTable(value.as<sol::table>(), parent + k + ".");
				break;
			}
		}
	}

	return result;
}

std::shared_ptr<SpineObject> Game::getObjectById(std::string objectId)
{
	if (objectId == "Player")
	{
		return player;
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
	return obj;
}

std::string Game::getLUAPath(std::string objectId)
{
	if (objectId == "Player")
	{
		return "player";
	}

	std::string scene = (*this->lua_state)["game"]["scene"];
	if (objectId == "Background")
	{
		return "scenes[\"" + scene + "\"][\"background\"]";
	}

	if ((*this->lua_state)["inventory_items"][objectId].valid())
	{
		return "inventory_items[\"" + objectId + "\"]";
	}
	else if ((*this->lua_state)["scenes"][scene]["items"][objectId].valid())
	{
		return "scenes[\"" + scene + "\"][\"items\"][\"" + objectId + "\"]";
	}
	return "";
}
