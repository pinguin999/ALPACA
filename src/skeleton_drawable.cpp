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
  // vertexArray(new VertexArray(Triangles, skeletonData->bonesCount * 4)),
  vertexEffect(nullptr), worldVertices(nullptr), clipper(nullptr) {
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
	// delete vertexArray;
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
	spSkeleton_update(skeleton, deltaTime);
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
	spSkeleton_update(skeleton, deltaTime);
	spAnimationState_update(state, deltaTime);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
}


void SkeletonDrawable::draw() const {
	// vertexArray->clear();
	// states.texture = 0;
	unsigned short quadIndices[6] = { 0, 1, 2, 2, 3, 0 };

	if (vertexEffect != nullptr) {
		vertexEffect->begin(vertexEffect, skeleton);
	}
	static spColor* blancColor = new spColor();

	jngl::Sprite* texture = nullptr;
	for (int j = 0; j < skeleton->slotsCount; ++j) {
		spSlot* slot = skeleton->drawOrder[j];
		spAttachment* attachment = slot->attachment;
		if (!attachment) continue;

		float* vertices = worldVertices;
		int verticesCount = 0;
		float* uvs = nullptr;
		unsigned short* indices = nullptr;
		int indicesCount = 0;
		spColor* attachmentColor = nullptr;

		if (attachment->type == SP_ATTACHMENT_REGION) {
			spRegionAttachment* regionAttachment = (spRegionAttachment*)attachment;
			spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, vertices, 0, 2);
			verticesCount = 4;
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
			verticesCount = mesh->super.worldVerticesLength >> 1;
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

		// vertex.color.r = r;
		// vertex.color.g = g;
		// vertex.color.b = b;
		// vertex.color.a = a;

		// spColor light;
		// light.r = r / 255.0f;
		// light.g = g / 255.0f;
		// light.b = b / 255.0f;
		// light.a = a / 255.0f;

		// TODO wieder rein
		// sf::BlendMode blend;
		// if (!usePremultipliedAlpha) {
		// 	switch (slot->data->blendMode) {
		// 		case BLEND_MODE_NORMAL:
		// 			blend = normal;
		// 			break;
		// 		case BLEND_MODE_ADDITIVE:
		// 			blend = additive;
		// 			break;
		// 		case BLEND_MODE_MULTIPLY:
		// 			blend = multiply;
		// 			break;
		// 		case BLEND_MODE_SCREEN:
		// 			blend = screen;
		// 			break;
		// 		default:
		// 			blend = normal;
		// 	}
		// } else {
		// 	switch (slot->data->blendMode) {
		// 		case BLEND_MODE_NORMAL:
		// 			blend = normalPma;
		// 			break;
		// 		case BLEND_MODE_ADDITIVE:
		// 			blend = additivePma;
		// 			break;
		// 		case BLEND_MODE_MULTIPLY:
		// 			blend = multiplyPma;
		// 			break;
		// 		case BLEND_MODE_SCREEN:
		// 			blend = screenPma;
		// 			break;
		// 		default:
		// 			blend = normalPma;
		// 	}
		// }

		// if (states.texture == 0) states.texture = texture;

		// if (states.blendMode != blend || states.texture != texture) {
		// target.draw(*vertexArray, states);
		// vertexArray->clear();
		// 	states.blendMode = blend;
		// 	states.texture = texture;
		// }

		// if (spSkeletonClipping_isClipping(clipper)) {
		// 	spSkeletonClipping_clipTriangles(clipper, vertices, verticesCount << 1, indices,
		// indicesCount, uvs, 2); 	vertices = clipper->clippedVertices->items; 	verticesCount =
		// clipper->clippedVertices->size >> 1; 	uvs = clipper->clippedUVs->items; 	indices =
		// clipper->clippedTriangles->items; 	indicesCount = clipper->clippedTriangles->size;
		// }

		// Vector2u size = texture->getSize();

		if (vertexEffect != nullptr) {
			spFloatArray_clear(tempUvs);
			spColorArray_clear(tempColors);
			for (int i = 0; i < verticesCount; i++) {
				// spColor vertexColor = light;
				spColor dark;
				dark.r = dark.g = dark.b = dark.a = 0;
				int index = i << 1;
				float x = vertices[index];
				float y = vertices[index + 1];
				float u = uvs[index];
				float v = uvs[index + 1];
				// vertexEffect->transform(vertexEffect, &x, &y, &u, &v, &vertexColor, &dark);
				vertices[index] = x;
				vertices[index + 1] = y;
				spFloatArray_add(tempUvs, u);
				spFloatArray_add(tempUvs, v);
				// spColorArray_add(tempColors, vertexColor);
			}

			// for (int i = 0; i < indicesCount; ++i) {
			// 	int index = indices[i] << 1;
			// 	// vertex.position.x = vertices[index];
			// 	// vertex.position.y = vertices[index + 1];
			// 	// vertex.texCoords.x = uvs[index] * size.x;
			// 	// vertex.texCoords.y = uvs[index + 1] * size.y;
			// 	spColor vertexColor = tempColors->items[index >> 1];
			// 	// vertex.color.r = static_cast<Uint8>(vertexColor.r * 255);
			// 	// vertex.color.g = static_cast<Uint8>(vertexColor.g * 255);
			// 	// vertex.color.b = static_cast<Uint8>(vertexColor.b * 255);
			// 	// vertex.color.a = static_cast<Uint8>(vertexColor.a * 255);
			// 	// vertexArray->append(vertex);
			// }
		} else {
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
		}

		spSkeletonClipping_clipEnd(clipper, slot);
	}
	// target.draw(*vertexArray, states);
	spSkeletonClipping_clipEnd2(clipper);

	if (vertexEffect != nullptr) {
		vertexEffect->end(vertexEffect);
	}
	jngl::setSpriteColor(255, 255, 255, 255);
}


spBoundingBoxAttachment *spSkeletonBounds_containsPointMatchingName(spSkeletonBounds *self, const std::string name, float x, float y) {
	int i;
	for (i = 0; i < self->count; ++i)
	{
		if(std::string(self->boundingBoxes[i]->super.super.name) == name)
			if (spPolygon_containsPoint(self->polygons[i], x, y)) return self->boundingBoxes[i];
	}
	return 0;
}

spBoundingBoxAttachment *spSkeletonBounds_containsPointNotMatchingName(spSkeletonBounds *self, const std::string name, float x, float y) {
	int i;
	for (i = 0; i < self->count; ++i)
	{
		if(std::string(self->boundingBoxes[i]->super.super.name) != name)
			if (spPolygon_containsPoint(self->polygons[i], x, y)) return self->boundingBoxes[i];
	}
	return 0;
}

} // namespace spine
