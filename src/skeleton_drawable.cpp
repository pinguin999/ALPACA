#include "skeleton_drawable.hpp"

#ifndef NDEBUG
void pac_unload_file(const char* path);

#endif

spine::SpineExtension* spine::getDefaultExtension() {
	return new spine::DefaultSpineExtension();
}

void TextureLoader::load(spine::AtlasPage& page, const spine::String& path) {
#ifndef NDEBUG
    pac_unload_file(path.buffer());
#endif

    auto texture = new jngl::Sprite(path.buffer());
    // if (!texture->loadFromFile(path)) return;

	// TODO
	// if (self->magFilter == SP_ATLAS_LINEAR) texture->setSmooth(true);
	// if (self->uWrap == SP_ATLAS_REPEAT && self->vWrap == SP_ATLAS_REPEAT)
	// texture->setRepeated(true);

	page.texture = texture;
	// Vector2u size = texture->getSize();
	page.width = texture->getWidth();
	page.height = texture->getHeight();
}

void TextureLoader::unload(void* texture) {
	delete (jngl::Sprite*)texture;
}

TextureLoader SkeletonDrawable::textureLoader;

SkeletonDrawable::SkeletonDrawable(spine::SkeletonData& skeletonData,
                                   std::unique_ptr<spine::AnimationStateData> animationStateData)
: timeScale(1), animationStateData(std::move(animationStateData)) {
	quadIndices.add(0);
	quadIndices.add(1);
	quadIndices.add(2);
	quadIndices.add(2);
	quadIndices.add(3);
	quadIndices.add(0);
	spine::Bone::setYDown(true);
	skeleton = std::make_unique<spine::Skeleton>(&skeletonData);

	if (!this->animationStateData) {
		this->animationStateData = std::make_unique<spine::AnimationStateData>(&skeletonData);
	}

	state = std::make_unique<spine::AnimationState>(this->animationStateData.get());
}

SkeletonDrawable::~SkeletonDrawable() = default;

void SkeletonDrawable::step() {
    const float deltaTime = 1.f / static_cast<float>(jngl::getStepsPerSecond());
	state->update(deltaTime * timeScale);
	state->apply(*skeleton);
	skeleton->updateWorldTransform(spine::Physics_Update);
}

void SkeletonDrawable::setAlpha(float alpha) {
	this->alpha = alpha;
}

void SkeletonDrawable::draw(const jngl::Mat3& modelview) const {

	static auto* blancColor = new spine::Color();

	jngl::Sprite* texture = nullptr;
	for (int j = 0; j < skeleton->getSlots().size(); ++j) {
		spine::Slot& slot = *skeleton->getDrawOrder()[j];
		spine::Attachment* attachment = slot.getAttachment();
		if (!attachment) {
			continue;
		}

		spine::Vector<float>* vertices = &worldVertices;
		int verticesCount = 0;
		spine::Vector<float>* uvs = nullptr;
		spine::Vector<unsigned short>* indices = nullptr;
		int indicesCount = 0;
		spine::Color* attachmentColor = nullptr;

		if (attachment->getRTTI().isExactly(spine::RegionAttachment::rtti)) {
			auto* regionAttachment = reinterpret_cast<spine::RegionAttachment*>(attachment);

			vertices->setSize(8, 0);
			regionAttachment->computeWorldVertices(slot, *vertices, 0, 2);
			verticesCount = 4;
			uvs = &regionAttachment->getUVs();
			indices = &quadIndices;
			indicesCount = 6;
			texture =
			    reinterpret_cast<jngl::Sprite*>(regionAttachment->getRegion()->rendererObject);
			attachmentColor = &regionAttachment->getColor();

		} else if (attachment->getRTTI().isExactly(spine::MeshAttachment::rtti)) {
			auto* mesh = reinterpret_cast<spine::MeshAttachment*>(attachment);
			attachmentColor = &mesh->getColor();

			// Early out if the slot color is 0
			if (attachmentColor->a == 0) {
				clipper.clipEnd(slot);
				continue;
			}

			texture = static_cast<jngl::Sprite*>(mesh->getRegion()->rendererObject);
			vertices->setSize(mesh->getWorldVerticesLength(), 0);
			mesh->computeWorldVertices(slot, 0, mesh->getWorldVerticesLength(), vertices->buffer(),
			                           0, 2);
			verticesCount = mesh->getWorldVerticesLength() >> 1;
			uvs = &mesh->getUVs();
			indices = &mesh->getTriangles();
			indicesCount = indices->size();
		} else if (attachment->getRTTI().isExactly(spine::ClippingAttachment::rtti)) {
			auto* clip = reinterpret_cast<spine::ClippingAttachment*>(slot.getAttachment());
			clipper.clipStart(slot, clip);
			continue;

		} else if (attachment->getRTTI().isExactly(spine::BoundingBoxAttachment::rtti)) {
#ifndef NDEBUG
            if (debugdraw) {
                auto* box = reinterpret_cast<spine::BoundingBoxAttachment*>(attachment);

                worldVertices.setSize(box->getWorldVerticesLength(), 0);
                box->computeWorldVertices(slot, 0, box->getWorldVerticesLength(), worldVertices.buffer(), 0, 2);

                float* bbvertices = worldVertices.buffer();
                int vertexCount = box->getWorldVerticesLength();

                if (std::string(box->getName().buffer()) == "non_walkable_area") {

                } else {
                    for (int i = 0; i < vertexCount - 2; i += 2) {
                        jngl::drawLine(modelview, { bbvertices[i], bbvertices[i + 1] }, { bbvertices[i + 2], bbvertices[i + 3] });
                    }
                    jngl::drawLine(modelview, { bbvertices[vertexCount - 2], bbvertices[vertexCount - 1] }, { bbvertices[0], bbvertices[1] });
                }
                jngl::Text bbname;
                bbname.setText(box->getName().buffer());
                bbname.setAlign(jngl::Alignment::CENTER);
                bbname.setCenter(bbvertices[0], bbvertices[1]);
                jngl::setFontColor(jngl::Rgba(0, 1, 0, 1));
                bbname.draw(modelview);
            }
#endif
            continue;
		} else {
			continue;
		}

		if (attachmentColor == nullptr) {
			attachmentColor = blancColor;
		}
		const auto r = static_cast<uint8_t>(skeleton->getColor().r * slot.getColor().r *
		                                    attachmentColor->r * 255);
		const auto g = static_cast<uint8_t>(skeleton->getColor().g * slot.getColor().g *
		                                    attachmentColor->g * 255);
		const auto b = static_cast<uint8_t>(skeleton->getColor().b * slot.getColor().b *
		                                    attachmentColor->b * 255);
		const auto a = static_cast<uint8_t>(skeleton->getColor().a * slot.getColor().a *
		                                    attachmentColor->a * 255);

		if (clipper.isClipping()) {
			clipper.clipTriangles(*vertices, *indices, *uvs, 2);
			vertices = &clipper.getClippedVertices();
			verticesCount = clipper.getClippedVertices().size() >> 1;
			uvs = &clipper.getClippedUVs();
			indices = &clipper.getClippedTriangles();
			indicesCount = clipper.getClippedTriangles().size();
		}

		std::vector<jngl::Vertex> vertexArray;
		for (int i = 0; i < indicesCount; ++i) {
			int const index = (*indices)[i] << 1;
			vertexArray.push_back(jngl::Vertex{
			    (*vertices)[index], (*vertices)[index + 1],
			    (*uvs)[index],     // * size.x
			    (*uvs)[index + 1], // * size.y
			});
		}
		if (r < 250 || g < 250 || b < 250) {
			jngl::setSpriteColor(r, g, b);
		}
		if (texture) {
			jngl::setSpriteColor(r, g, b, a * alpha);
			texture->drawMesh(modelview, vertexArray);
			jngl::setSpriteColor(255, 255, 255, 255);
		}

		clipper.clipEnd(slot);
	}
	// target.draw(*vertexArray, states);
	clipper.clipEnd();

	jngl::setSpriteColor(255, 255, 255, 255);

	// Alternative simpler method:
	// spine::RenderCommand* command = SkeletonRenderer::handle().spine.render(*skeleton);
	// while (command) {
	// 	std::vector<jngl::Vertex> vertices;
	// 	float* positions = command->positions;
	// 	float* uvs = command->uvs;
	// 	uint32_t* colors = command->colors;
	// 	uint16_t* indices = command->indices;
	// 	jngl::Sprite* texture = (jngl::Sprite*)command->texture;
	// 	for (int i = 0, n = command->numIndices; i < n; ++i) {
	// 		int ii = indices[i];
	// 		int index = ii << 1;
	// 		jngl::Vertex vertex;
	// 		vertex.x = positions[index];
	// 		vertex.y = positions[index + 1];
	// 		vertex.u = uvs[index];
	// 		vertex.v = uvs[index + 1];
	// 		//  vertex.color = colors[i];
	// 		vertices.push_back(vertex);
	// 	}
	// 	//   BlendMode blendMode = command->blendMode; // Spine blend mode equals engine blend mode
	// 	texture->drawMesh(vertices);
	// 	command = command->next;
	// }
}

spine::BoundingBoxAttachment *spSkeletonBounds_containsPointMatchingName(spine::SkeletonBounds *self, const std::string &name, float x, float y) {
	spine::Vector<spine::BoundingBoxAttachment*>& boundingBoxes = self->getBoundingBoxes();
	spine::Vector<spine::Polygon*>& polygons = self->getPolygons();
	for (size_t i = 0; i < boundingBoxes.size(); ++i)
	{
        if (std::string(boundingBoxes[i]->getName().buffer()) == name)
        {
            if (self->containsPoint(polygons[i], x, y))
            {
                return boundingBoxes[i];
            }
        }
    }
    return nullptr;
}

spine::BoundingBoxAttachment *spSkeletonBounds_containsPointNotMatchingName(spine::SkeletonBounds *self, const std::string &name, float x, float y) {
	spine::Vector<spine::BoundingBoxAttachment*>& boundingBoxes = self->getBoundingBoxes();
	spine::Vector<spine::Polygon*>& polygons = self->getPolygons();
	for (size_t i = 0; i < boundingBoxes.size(); ++i)
	{
        if (std::string(boundingBoxes[i]->getName().buffer()) != name)
        {
            if (self->containsPoint(polygons[i], x, y))
            {
                return boundingBoxes[i];
            }
        }
    }
    return nullptr;
}
