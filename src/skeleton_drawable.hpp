#pragma once

#include <jngl.hpp>

#include <spine/spine.h>
#include <spine/extension.h>

_SP_ARRAY_DECLARE_TYPE(spColorArray, spColor)

namespace spine {

class SkeletonDrawable {
public:
	spSkeleton* skeleton;
	spAnimationState* state;
	float timeScale;

	explicit SkeletonDrawable(spSkeletonData* skeleton, spAnimationStateData* stateData = 0);
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
	float* worldVertices;
	spFloatArray* tempUvs;
	spColorArray* tempColors;
	spSkeletonClipping* clipper;
};


spBoundingBoxAttachment *spSkeletonBounds_containsPointMatchingName(spSkeletonBounds *self, const std::string &name, float x, float y);
spBoundingBoxAttachment *spSkeletonBounds_containsPointNotMatchingName(spSkeletonBounds *self, const std::string &name, float x, float y);

} // namespace spine
