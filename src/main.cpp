#include <jngl/init.hpp>
#include "game.hpp"

#ifndef NDEBUG
void pac_unload_file(const char* path)
{
	jngl::unload(path);
}
#endif

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
	auto args = jngl::getArgs();
	if (args.size() == 1 && args[0] == "--fullscreen") {
		params.fullscreen = true;
	}
	std::srand(std::time(nullptr));

	std::optional<YAML::Node> config;
	std::stringstream fin = jngl::readAsset("config/game.json");
	if (fin)
	{
		config = YAML::Load(fin.str());
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
