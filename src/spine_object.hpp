#pragma once

#include <memory>
#include <map>
#include <jngl/Vec2.hpp>
#include <spine/spine.h>
#include "skeleton_drawable.hpp"
#include <sol/sol.hpp>

struct spSkeletonData;
class Game;

namespace spine
{
	class SkeletonDrawable;
} // namespace spine

/// Basisklasse für die Spine Objekte im Spiel
class SpineObject : public std::enable_shared_from_this<SpineObject>
{
public:
	SpineObject(const std::shared_ptr<Game> &game, const std::string &spine_file, const std::string &id, float scale = 1);
	virtual ~SpineObject()
	{
		spSkeletonBounds_dispose(bounds);
		spAnimationStateData_dispose(animationStateData);
		spSkeletonData_dispose(skeletonData);
		spAtlas_dispose(atlas);
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
		skeleton->skeleton->scaleX = scale;
		skeleton->skeleton->scaleY = scale;
	}

	void activate();
	void setVisible(bool visible) { this->visible = visible; }
	bool getVisible() { return visible; }

	std::unique_ptr<spine::SkeletonDrawable> skeleton;
	spSkeletonBounds *bounds = nullptr;
	spSkeletonData *skeletonData = nullptr;
	spAnimationStateData *animationStateData = nullptr;
	spAtlas *atlas = nullptr;
	std::optional<jngl::Vec2> getPoint(const std::string &point_name) const;
	void playAnimation(int trackIndex, const std::string &currentAnimation, bool loop, sol::function callback);
	void addAnimation(int trackIndex, const std::string &currentAnimation, bool loop, float delay, sol::function callback);
	void onAnimationComplete(const std::string &key);
	void setSkin(const std::string &skin) const;
	std::vector<std::string> getPointNames() const;
	bool cross_scene = false;
	bool abs_position = false;

	static void animationStateListener(spAnimationState *state, spEventType type, spTrackEntry *entry, spEvent *event);
	std::string collision_script = ""; // TODO protected
	std::string getName() { return spine_name; };
	std::string getId() { return id; };
	double getZ() const;
	int layer = 1;
	void setDeleted() { deleted = true; };

protected:
	std::string currentAnimation = "idle";
	std::map<std::string, sol::function> animation_callback;
	sol::function walk_callback;

	bool deleted = false;
	bool visible = true;
	jngl::Vec2 position;
	float scale = 1.0;
	float rotation = 0.0;
	std::string spine_name;
	std::string id;
	const std::weak_ptr<Game> game;
	std::shared_ptr<SpineObject> parent = nullptr;
};
