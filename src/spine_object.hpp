#pragma once

#include <memory>
#include <map>
#include <jngl/Vec2.hpp>
#include <spine/spine.h>
#include "skeleton_drawable.hpp"
#include <sol/sol.hpp>
#include "lua_callback.hpp"

struct spSkeletonData;
class Game;

/// Basisklasse für die Spine Objekte im Spiel
class SpineObject : public std::enable_shared_from_this<SpineObject>
{
public:
	SpineObject(const std::shared_ptr<Game> &game, const std::string &spine_file, std::string id, float scale = 1);
	virtual ~SpineObject()
	{
	}

	std::shared_ptr<SpineObject> getptr()
	{
		return shared_from_this();
	}

	/// true zurückgeben damit das Objekt entfernt wird
	virtual bool step(bool force = false) = 0;

	virtual void draw() const = 0;

	jngl::Vec2 getPosition() { return position; }
	void setPosition(jngl::Vec2 position) { this->position = position; }

	std::shared_ptr<SpineObject> getParent() { return parent; }
	void setParent(std::shared_ptr<SpineObject> parent) { this->parent = parent; }

	float getRotation() const { return rotation; }
	void setRotation(const float rotation) { this->rotation = rotation; }

	float getScale() const { return scale; }
	void setScale(const float scale)
	{
		this->scale = scale;
		skeleton->skeleton->setScaleX(scale);
		skeleton->skeleton->setScaleY(scale);
	}

	void setVisible(bool visible) { this->visible = visible; }
	bool getVisible() { return visible; }

	std::unique_ptr<SkeletonDrawable> skeleton;
	std::unique_ptr<spine::SkeletonBounds> bounds;
	std::unique_ptr<spine::SkeletonData> skeletonData;
	std::unique_ptr<spine::Atlas> atlas;
	std::optional<jngl::Vec2> getPoint(const std::string &point_name) const;
	void playAnimation(int trackIndex, const std::string &currentAnimation, bool loop, std::optional<sol::function> callback = std::nullopt);
	void stopAnimation(int trackIndex);
	void addAnimation(int trackIndex, const std::string &currentAnimation, bool loop, float delay, std::optional<sol::function> callback = std::nullopt);
	void onAnimationComplete(int index, const std::string &animation);
	void setSkin(const std::string &skin);
	void setSkins(const std::vector<std::string> &skins);
	std::vector<std::string> getPointNames() const;
	bool abs_position = false;

	std::string collision_script = ""; // TODO protected
	std::string getName() { return spine_name; };
	std::string getId() { return id; };
	virtual double getZ() const;
	int layer = 1;
	void setDeleted() { deleted = true; };
	void toLuaState();
    bool getCrossScene() const {return cross_scene;};
    void setCrossScene(bool cross_scene);

protected:
	std::string currentAnimation = "idle";
	std::map<std::string, LuaCallback> animation_callback;
	std::optional<LuaCallback> walk_callback;

	bool cross_scene = false;
	bool deleted = false;
	bool visible = true;
	std::vector<std::string> skins = {"default"};
	std::unique_ptr<spine::Skin> combinedSkin;
	jngl::Vec2 position;
	float scale = 1.0;
	float rotation = 0.0;
	std::string spine_name;
	std::string id;
	const std::weak_ptr<Game> game;
	std::shared_ptr<SpineObject> parent = nullptr;
};
