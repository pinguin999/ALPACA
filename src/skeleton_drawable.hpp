#pragma once

#include <jngl.hpp>

#include <spine/spine.h>

class TextureLoader : public spine::TextureLoader {
public:
	void load(spine::AtlasPage& page, const spine::String& path) override;
	void unload(void* texture) override;
};


class SkeletonDrawable {
public:
	static TextureLoader textureLoader;
	std::unique_ptr<spine::Skeleton> skeleton;
	std::unique_ptr<spine::AnimationState> state;
	float timeScale;

	explicit SkeletonDrawable(spine::SkeletonData& skeleton,
	                          std::unique_ptr<spine::AnimationStateData> = nullptr);
	~SkeletonDrawable();

    void step();
	void setAlpha(float);

	void draw(const jngl::Mat3& modelview = jngl::modelview()) const;

#ifndef NDEBUG
	bool debugdraw = false;
#endif

private:
	std::unique_ptr<spine::AnimationStateData> animationStateData;
	mutable spine::Vector<float> worldVertices;
	mutable spine::Vector<unsigned short> quadIndices;
	mutable spine::SkeletonClipping clipper;
	float alpha = 1.f;
};


spine::BoundingBoxAttachment *spSkeletonBounds_containsPointMatchingName(spine::SkeletonBounds *self, const std::string &name, float x, float y);
spine::BoundingBoxAttachment *spSkeletonBounds_containsPointNotMatchingName(spine::SkeletonBounds *self, const std::string &name, float x, float y);
