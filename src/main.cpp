#include <jngl/init.hpp>
#include "game.hpp"

#include <fstream>

class QuitWithEscape : public jngl::Job
{
public:
	void step() override
	{
		if (jngl::keyPressed(jngl::key::Escape))
		{
			jngl::quit();
		}
	}
	void draw() const override
	{
	}
};

jngl::AppParameters jnglInit()
{

	jngl::AppParameters params;
	std::srand(std::time(nullptr));

	std::optional<YAML::Node> config;
	std::ifstream fin("config/game.json");
	if (fin)
	{
		std::ostringstream tmp;
		tmp << fin.rdbuf();
		tmp.str();

		config = YAML::Load(tmp.str());
		params.displayName = (*config)["name"].as<std::string>();
#ifndef NDEBUG
		params.screenSize = {double((*config)["screenSize"]["x"].as<int>()), double((*config)["screenSize"]["y"].as<int>())};
#endif
		params.minAspectRatio = {double((*config)["minAspectRatio"]["x"].as<int>()), double((*config)["minAspectRatio"]["y"].as<int>())};
		params.maxAspectRatio = {double((*config)["maxAspectRatio"]["x"].as<int>()), double((*config)["maxAspectRatio"]["y"].as<int>())};
	}

	params.start = [tmp = std::move(config)]() mutable
	{
		if (!tmp)
		{
			tmp = YAML::Load(jngl::readAsset("config/game.json").str());
		}
		YAML::Node &config = *tmp;
#ifndef NDEBUG
		jngl::addJob(std::make_shared<QuitWithEscape>());
#endif
		jngl::setFont(config["default_font"].as<std::string>());
		jngl::setAntiAliasing(config["antiAliasing"].as<bool>());
		jngl::setIcon(config["icon"].as<std::string>());
		auto game = std::make_shared<Game>(config);
		game->init();
		return game;
	};

	return params;
}
