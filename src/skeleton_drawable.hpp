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
	spine::Skeleton* skeleton;
	spine::AnimationState* state;
	float timeScale;

	explicit SkeletonDrawable(spine::SkeletonData* skeleton,
	                          spine::AnimationStateData* stateData = nullptr);
	~SkeletonDrawable();

    void endAnimation(int trackIndex) const;

    void step();

	void draw(const jngl::Mat3& modelview = jngl::modelview()) const;

	void setAlpha(float);

#ifndef NDEBUG
	bool debugdraw = false;
#endif

private:
	float alpha = 1.f;
	bool ownsAnimationStateData;
	mutable spine::Vector<float> worldVertices;
	mutable spine::Vector<unsigned short> quadIndices;
	mutable spine::SkeletonClipping clipper;
};

spine::BoundingBoxAttachment *spSkeletonBounds_containsPointMatchingName(spine::SkeletonBounds *self, const std::string &name, float x, float y);
spine::BoundingBoxAttachment *spSkeletonBounds_containsPointNotMatchingName(spine::SkeletonBounds *self, const std::string &name, float x, float y);
