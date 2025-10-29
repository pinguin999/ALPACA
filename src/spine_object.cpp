#include "spine_object.hpp"
#include "game.hpp"

// void SpineObject::animationStateListener(spAnimationState *state, spEventType type, spTrackEntry *entry,
//                                          spEvent *event)
// {
//     if (event)
//     {
//         if (event->data->audioPath)
//         {
//             jngl::debug(event->data->audioPath);
//             jngl::play("audio/" + std::string(event->data->audioPath));
//         }
//         if (event->data->stringValue)
//         {
//             // This in Lua setzen

//             jngl::debug(event->data->stringValue);

//             if (auto _game = reinterpret_cast<SpineObject *>(state->userData)->game.lock())
//             {

//                 (*_game->lua_state)["this"] = reinterpret_cast<SpineObject *>(state->userData)->getptr();
//                 std::string extension = ".lua";
//                 std::string event_string = std::string(event->data->stringValue);
//                 if (std::equal(extension.rbegin(), extension.rend(), event_string.rbegin()))
//                 {
//                     // run lua script from file
//                      _game->runAction(event_string.erase(event_string.size() - 4), (*_game->lua_state)["this"]);
//                 }
//                 else
//                 {
//                     _game->lua_state->script(event_string);
//                 }
//             }
//         }
//     }

//     switch (type)
//     {
//     case SP_ANIMATION_INTERRUPT:
//         break;

//     case SP_ANIMATION_COMPLETE:
//         if (!entry->loop)
//         {
//             reinterpret_cast<SpineObject *>(state->userData)->onAnimationComplete(std::to_string(entry->trackIndex) + std::string(entry->animation->name));
//         }
//         break;
//     default:
//         break;
//     }
// }

SpineObject::SpineObject(const std::shared_ptr<Game> &game, const std::string &spine_file, std::string id, float scale) : walk_callback((*game->lua_state)["pass"]), scale(scale), spine_name(spine_file), id(std::move(id)), game(game)
{
    atlas = new spine::Atlas((spine_file + "/" + spine_file + ".atlas").c_str(), &SkeletonDrawable::textureLoader);
    assert(atlas);
    auto* json = new spine::SkeletonJson(atlas);
    json->setScale(scale);

#ifndef NDEBUG
    while (true)
    {
#endif
        skeletonData = json->readSkeletonDataFile((spine_file + "/" + spine_file + ".json").c_str());
        if (!skeletonData)
        {
            jngl::error("Fatal Error loading " + spine_file + ": " + json->error);
#ifndef NDEBUG
            atlas = new spine::Atlas((spine_file + "/" + spine_file + ".atlas").c_str(), &SkeletonDrawable::textureLoader);
            json = new spine::SkeletonJson(atlas);
            json->setScale(scale);
            // assert(skeletonData);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        break;
#endif
    }

    delete json;

    animationStateData = new spine::AnimationStateData(skeletonData);
    skeleton = std::make_unique<SkeletonDrawable>(skeletonData, animationStateData);
    skeleton->state->userData = this;
    skeleton->state->listener = static_cast<spAnimationStateListener>(&this->animationStateListener);
    bounds = spSkeletonBounds_create();

    skeleton->step();
}

std::optional<jngl::Vec2> SpineObject::getPoint(const std::string &point_name) const
{
    auto *slot = spSkeleton_findSlot(skeleton->skeleton, point_name.c_str());
    if (!slot)
    {
        return std::nullopt;
    }
    // Possible Problem: Slot and Point have to have the same name
    auto *att = spSkeleton_getAttachmentForSlotName(skeleton->skeleton, point_name.c_str(), point_name.c_str());
    spPointAttachment *point = SUB_CAST(spPointAttachment, att);
    float x = 0;
    float y = 0;
    if (point && point->super.type == SP_ATTACHMENT_POINT)
    {
        spPointAttachment_computeWorldPosition(point, slot->bone, &x, &y);
        return jngl::Vec2(x, y);
    }
    return std::nullopt;
}

std::vector<std::string> SpineObject::getPointNames() const
{
    std::vector<std::string> result;

    for (int i = 0; i < skeleton->skeleton->data->skinsCount; i++)
    {
        spSkin *skin = skeleton->skeleton->data->skins[i];
        spSkinEntry *entry = spSkin_getAttachments(skin);
        while (entry)
        {
            if (entry->attachment && entry->attachment->type == SP_ATTACHMENT_POINT)
            {
                result.emplace_back(entry->attachment->name);
            }
            entry = entry->next;
        }
    }
    sort(result.begin(), result.end());
    return result;
}

void SpineObject::playAnimation(int trackIndex, const std::string &currentAnimation, bool loop, sol::function callback)
{
    this->animation_callback[std::to_string(trackIndex) + currentAnimation] = std::move(callback);
    if (trackIndex == 0)
    {
        this->currentAnimation = currentAnimation;
    }
    spine::Animation *animation = skeleton->state->getData()->getSkeletonData()->findAnimation(currentAnimation.c_str());
    if (animation)
    {
        auto entry = skeleton->state->setAnimation(trackIndex, currentAnimation.c_str(), static_cast<int>(loop));
        entry->setListener([this](spine::AnimationState*, spine::EventType type,
		                          spine::TrackEntry* entry, spine::Event* event) {
			if (event)
    {
        if (event->getData().getAudioPath() != "")
        {
            jngl::debug(event->getData().getAudioPath().buffer());
            jngl::play("audio/" + std::string(event->getData().getAudioPath().buffer()));
        }
        if (event->getData().getStringValue() != "")
        {
            // This in Lua setzen

            jngl::debug(event->getData().getStringValue().buffer());

            if (auto _game = this->game.lock())
            {

                (*_game->lua_state)["this"] = this->getptr();
                std::string extension = ".lua";
                std::string event_string = std::string(event->getData().getStringValue().buffer());
                if (std::equal(extension.rbegin(), extension.rend(), event_string.rbegin()))
                {
                    // run lua script from file
                     _game->runAction(event_string.erase(event_string.size() - 4), (*_game->lua_state)["this"]);
                }
                else
                {
                    _game->lua_state->script(event_string);
                }
            }
        }
    }

    switch (type)
    {
    case spine::EventType_Interrupt:
        break;

    case spine::EventType_Complete:
        if (!entry->getLoop())
        {
            onAnimationComplete(std::to_string(entry->getTrackIndex()) + std::string(entry->getAnimation()->getName().buffer()));
        }
        break;
    default:
        break;
    }
		});
    }
    else
    {
        jngl::error("\033[1;31m The animation " + currentAnimation + " is missing for " + spine_name + " \033[0m");
    }
}


void SpineObject::stopAnimation(int trackIndex)
{
    if (auto _game = game.lock()) {
        skeleton->state->setEmptyAnimation(trackIndex, 0.1f);
        animation_callback[std::to_string(trackIndex) + "<empty>"] = (*_game->lua_state)["pass"];
    }
}


void SpineObject::addAnimation(int trackIndex, const std::string &currentAnimation, bool loop, float delay, sol::function callback)
{
    this->animation_callback[std::to_string(trackIndex) + currentAnimation] = std::move(callback);
    if (trackIndex == 0)
    {
        this->currentAnimation = currentAnimation;
    }
    spine::Animation *animation = skeleton->state->getData()->getSkeletonData()->findAnimation(currentAnimation.c_str());
    if (animation)
    {
        skeleton->state->addAnimation(trackIndex, animation, static_cast<int>(loop), delay);
    }
    else
    {
        jngl::error("\033[1;31m The animation " + currentAnimation + " is missing for " + spine_name + " \033[0m");
    }
}

void SpineObject::onAnimationComplete(const std::string &key)
{
    if (auto _game = game.lock())
    {
        if (!deleted)
        {
            // Set animation back to default animation in Lua state
            const std::string lua_object = _game->getLuaPath(getId());
            std::string animation = (*_game->lua_state)["config"]["spine_default_animation"];
            (*_game->lua_state).script(lua_object + ".animation = \"" + animation + "\"");
            (*_game->lua_state).script(lua_object + ".loop_animation = true");
        }

        animation_callback[key]();
        animation_callback[key] = (*_game->lua_state)["pass"];
    }
}

void SpineObject::setSkin(const std::string &skin)
{
    this->skin = skin;
    const int resault = skeleton->skeleton->setSkin(skin.c_str());
    skeleton->skeleton->setToSetupPose();
    if (!resault)
    {
        jngl::error("\033[1;31m The Skin " + skin + " is missing for " + spine_name + " \033[0m");
    }
}


double SpineObject::getZ() const
{
    return position.y + layer * 2000.0;
}


void SpineObject::toLuaState()
{
    if (auto _game = game.lock()) {
        std::string scene = (*_game->lua_state)["game"]["scene"];
        if (cross_scene) {
            scene = "cross_scene";
        }

        (*_game->lua_state)["scenes"][scene]["items"][id] =
            _game->lua_state->create_table_with(
                "spine", spine_name,
                "object", shared_from_this(),
                "x", position.x,
                "y", position.y,
                "animation", currentAnimation,
                "loop_animation", true,
                "visible", visible,
                "cross_scene", cross_scene,
                "abs_position", abs_position,
                "layer", layer,
                "skin", skin,
                "scale", scale);
    }
}

void SpineObject::setCrossScene(bool cross_scene)
{
    this->cross_scene = cross_scene;
}
