#include "skeleton_drawable.hpp"
#include "game.hpp"

#ifndef SPINE_MESH_VERTEX_COUNT_MAX
#define SPINE_MESH_VERTEX_COUNT_MAX 1000
#endif

bool operator==(const spColor& lhs, const spColor& rhs)
{
    return lhs.a == rhs.a && lhs.r == rhs.r &&lhs.g == rhs.g &&lhs.b == rhs.b;
}

_SP_ARRAY_IMPLEMENT_TYPE(spColorArray, spColor)

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path) {
	jngl::Sprite* texture;
	try
	{
		texture = new jngl::Sprite(path);
	}catch(...)
	{
		unsigned char color[] = {255, 255, 0, 255};
		texture = new jngl::Sprite(color, 1, 1);
	}

	texture->setPos(0, 0);
	// if (!texture->loadFromFile(path)) return;

	// TODO
	// if (self->magFilter == SP_ATLAS_LINEAR) texture->setSmooth(true);
	// if (self->uWrap == SP_ATLAS_REPEAT && self->vWrap == SP_ATLAS_REPEAT)
	// texture->setRepeated(true);

	self->rendererObject = texture;
	// Vector2u size = texture->getSize();
	self->width = texture->getWidth();
	self->height = texture->getHeight();
}

void _spAtlasPage_disposeTexture(spAtlasPage* self) {
	delete (jngl::Sprite*)self->rendererObject;
}

char* _spUtil_readFile(const char* path, int* length) {
	const auto str = jngl::readAsset(path).str();
	if (length) {
		*length = static_cast<int>(str.length());
	}
	char* buf = static_cast<char*>(malloc(str.length() + 1));
	std::copy(str.begin(), str.end(), buf);
	buf[str.length()] = '\0';
	return buf;
}

namespace spine {

SkeletonDrawable::SkeletonDrawable(spSkeletonData* skeletonData, spAnimationStateData* stateData)
: timeScale(1),
  worldVertices(nullptr), clipper(nullptr) {
	spBone_setYDown(true);
	worldVertices = MALLOC(float, SPINE_MESH_VERTEX_COUNT_MAX);
	skeleton = spSkeleton_create(skeletonData);
	tempUvs = spFloatArray_create(16);
	tempColors = spColorArray_create(16);

	ownsAnimationStateData = stateData == nullptr;
	if (ownsAnimationStateData) stateData = spAnimationStateData_create(skeletonData);

	state = spAnimationState_create(stateData);

	clipper = spSkeletonClipping_create();
}

SkeletonDrawable::~SkeletonDrawable() {
	FREE(worldVertices);
	if (ownsAnimationStateData) spAnimationStateData_dispose(state->data);
	spAnimationState_dispose(state);
	spSkeleton_dispose(skeleton);
	spSkeletonClipping_dispose(clipper);
	spFloatArray_dispose(tempUvs);
	spColorArray_dispose(tempColors);
}

void SkeletonDrawable::step() {
	const float deltaTime = 1.f / float(jngl::getStepsPerSecond());
	spAnimationState_update(state, deltaTime * timeScale);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
}

void SkeletonDrawable::endAnimation(int trackIndex)
{
	auto animation = spAnimationState_getCurrent(state, trackIndex);
	if(!animation)
		return;
	float deltaTime = spTrackEntry_getTrackComplete(animation);
	spAnimationState_update(state, deltaTime);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
}


void SkeletonDrawable::draw() const {
	unsigned short quadIndices[6] = { 0, 1, 2, 2, 3, 0 };

	static spColor* blancColor = new spColor();

	jngl::Sprite* texture = nullptr;
	for (int j = 0; j < skeleton->slotsCount; ++j) {
		spSlot* slot = skeleton->drawOrder[j];
		spAttachment* attachment = slot->attachment;
		if (!attachment) continue;

		float* vertices = worldVertices;
		float* uvs = nullptr;
		unsigned short* indices = nullptr;
		int indicesCount = 0;
		spColor* attachmentColor = nullptr;

		if (attachment->type == SP_ATTACHMENT_REGION) {
			spRegionAttachment* regionAttachment = (spRegionAttachment*)attachment;
			spRegionAttachment_computeWorldVertices(regionAttachment, slot, vertices, 0, 2);
			uvs = regionAttachment->uvs;
			indices = quadIndices;
			indicesCount = 6;
			texture = (jngl::Sprite*)((spAtlasRegion*)regionAttachment->rendererObject)
			              ->page->rendererObject;
			attachmentColor = &regionAttachment->color;

		} else if (attachment->type == SP_ATTACHMENT_MESH) {
			spMeshAttachment* mesh = (spMeshAttachment*)attachment;
			if (mesh->super.worldVerticesLength > SPINE_MESH_VERTEX_COUNT_MAX) continue;
			texture = (jngl::Sprite*)((spAtlasRegion*)mesh->rendererObject)->page->rendererObject;
			spVertexAttachment_computeWorldVertices(
			    SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, worldVertices, 0, 2);
			uvs = mesh->uvs;
			indices = mesh->triangles;
			indicesCount = mesh->trianglesCount;
			attachmentColor = &mesh->color;
		} else if (attachment->type == SP_ATTACHMENT_CLIPPING) {
			spClippingAttachment* clip = (spClippingAttachment*)slot->attachment;
			spSkeletonClipping_clipStart(clipper, slot, clip);
			continue;
		}else if(attachment->type == SP_ATTACHMENT_BOUNDING_BOX){
#ifndef NDEBUG
		if(debugdraw)
		{
			float* bbvertices = worldVertices;

			spBoundingBoxAttachment* box = (spBoundingBoxAttachment*)attachment;

			spVertexAttachment_computeWorldVertices(SUPER(box), slot, 0, box->super.verticesCount, bbvertices, 0, 2);
			for(int i = 0; i < box->super.verticesCount - 2; i+=2){
				jngl::drawLine(bbvertices[i], bbvertices[i+1], bbvertices[i+2], bbvertices[i+3]);
			}
			jngl::drawLine(bbvertices[box->super.verticesCount -2], bbvertices[box->super.verticesCount - 1], bbvertices[0], bbvertices[1]);
			jngl::Text bbname;
			bbname.setText(box->super.super.name);
			bbname.setAlign(jngl::Alignment::CENTER);
			bbname.setCenter(bbvertices[0], bbvertices[1]);
			bbname.draw();
		}
#endif

		} else
			continue;

		 if (attachmentColor == nullptr)
		 {
		 	attachmentColor = blancColor;
		 }
		 const auto r =
		 	static_cast<uint8_t>(skeleton->color.r * slot->color.r * attachmentColor->r * 255);
		 const auto g =
		 	static_cast<uint8_t>(skeleton->color.g * slot->color.g * attachmentColor->g * 255);
		 const auto b =
		 	static_cast<uint8_t>(skeleton->color.b * slot->color.b * attachmentColor->b * 255);
		 const auto a =
		 	static_cast<uint8_t>(skeleton->color.a * slot->color.a * attachmentColor->a * 255);


			std::vector<jngl::Vertex> vertexArray;
			for (int i = 0; i < indicesCount; ++i) {
				int index = indices[i] << 1;
				vertexArray.push_back(jngl::Vertex{
				    vertices[index], vertices[index + 1],
				    uvs[index],     // * size.x
				    uvs[index + 1], // * size.y
				});
			}
			if (r < 250 || g < 250 || b < 250) {
				jngl::setSpriteColor(r, g, b);
			}
			if (texture)
			{
				jngl::setSpriteColor(r, g, b, a);
				texture->drawMesh(vertexArray);
				jngl::setSpriteColor(255, 255, 255, 255);
			}

		spSkeletonClipping_clipEnd(clipper, slot);
	}
	// target.draw(*vertexArray, states);
	spSkeletonClipping_clipEnd2(clipper);

	jngl::setSpriteColor(255, 255, 255, 255);
}


spBoundingBoxAttachment *spSkeletonBounds_containsPointMatchingName(spSkeletonBounds *self, const std::string &name, float x, float y) {
	int i;
	for (i = 0; i < self->count; ++i)
	{
		if(std::string(self->boundingBoxes[i]->super.super.name) == name)
			if (spPolygon_containsPoint(self->polygons[i], x, y)) return self->boundingBoxes[i];
	}
	return 0;
}

spBoundingBoxAttachment *spSkeletonBounds_containsPointNotMatchingName(spSkeletonBounds *self, const std::string &name, float x, float y) {
	int i;
	for (i = 0; i < self->count; ++i)
	{
		if(std::string(self->boundingBoxes[i]->super.super.name) != name)
			if (spPolygon_containsPoint(self->polygons[i], x, y)) return self->boundingBoxes[i];
	}
	return 0;
}

} // namespace spine
