#include "spine_object.hpp"
#include "game.hpp"

void SpineObject::animationStateListener(spAnimationState *state, spEventType type, spTrackEntry *entry,
                                         spEvent *event)
{
    if (event)
    {
        if (event->data->audioPath)
        {
            jngl::debugLn(event->data->audioPath);
            jngl::play("audio/" + std::string(event->data->audioPath));
        }
        if (event->data->stringValue)
        {
            // This in Lua setzen

            jngl::debugLn(event->data->stringValue);

            if (auto _game = reinterpret_cast<SpineObject *>(state->userData)->game.lock())
            {

                (*_game->lua_state)["this"] = reinterpret_cast<SpineObject *>(state->userData)->getptr();
                std::string extension = ".lua";
                std::string event_string = std::string(event->data->stringValue);
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
    case SP_ANIMATION_INTERRUPT:
        break;

    case SP_ANIMATION_COMPLETE:
        if (!entry->loop)
        {
            reinterpret_cast<SpineObject *>(state->userData)->onAnimationComplete(std::to_string(entry->trackIndex) + std::string(entry->animation->name));
        }
        break;
    default:
        break;
    }
}

SpineObject::SpineObject(const std::shared_ptr<Game> &game, const std::string &spine_file, const std::string &id, float scale) : walk_callback((*game->lua_state)["pass"]), scale(scale), spine_name(spine_file), id(id), game(game)
{
    atlas = spAtlas_createFromFile((spine_file + "/" + spine_file + ".atlas").c_str(), nullptr);
    assert(atlas);
    spSkeletonJson *json = spSkeletonJson_create(atlas);
    json->scale = scale;

#ifndef NDEBUG
    while (true)
    {
#endif
        skeletonData = spSkeletonJson_readSkeletonDataFile(json, (spine_file + "/" + spine_file + ".json").c_str());
        if (!skeletonData)
        {
            jngl::debugLn("Fatal Error loading " + spine_file + ": " + json->error);
#ifndef NDEBUG
            atlas = spAtlas_createFromFile((spine_file + "/" + spine_file + ".atlas").c_str(), nullptr);
            json = spSkeletonJson_create(atlas);
            json->scale = scale;
            // assert(skeletonData);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        break;
#endif
    }

    spSkeletonJson_dispose(json);

    animationStateData = spAnimationStateData_create(skeletonData);
    skeleton = std::make_unique<spine::SkeletonDrawable>(skeletonData, animationStateData);
    skeleton->state->userData = this;
    skeleton->state->listener = static_cast<spAnimationStateListener>(&this->animationStateListener);
    playAnimation(0, game->config["spine_default_animation"].as<std::string>(), true, (*game->lua_state)["pass"]);
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
    if (point)
    {
        spPointAttachment_computeWorldPosition(point, slot->bone, &x, &y);
    }
    return jngl::Vec2(x, y);
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
    spAnimation *animation = spSkeletonData_findAnimation(skeleton->state->data->skeletonData, currentAnimation.c_str());
    if (animation)
    {
        spAnimationState_setAnimation(skeleton->state, trackIndex, animation, static_cast<int>(loop));
    }
    else
    {
        jngl::debugLn("\033[1;31m The animation " + currentAnimation + " is missing! \033[0m");
    }
}

void SpineObject::addAnimation(int trackIndex, const std::string &currentAnimation, bool loop, float delay, sol::function callback)
{
    this->animation_callback[std::to_string(trackIndex) + currentAnimation] = std::move(callback);
    if (trackIndex == 0)
    {
        this->currentAnimation = currentAnimation;
    }
    spAnimation *animation = spSkeletonData_findAnimation(skeleton->state->data->skeletonData, currentAnimation.c_str());
    if (animation)
    {
        spAnimationState_addAnimation(skeleton->state, trackIndex, animation, static_cast<int>(loop), delay);
    }
    else
    {
        jngl::debugLn("\033[1;31m The animation " + currentAnimation + " is missing! \033[0m");
    }
}

void SpineObject::onAnimationComplete(const std::string &key)
{
    if (auto _game = game.lock())
    {
        // Set animation back to default animation in Lua state
        const std::string lua_object = _game->getLuaPath(getId());
        (*_game->lua_state).script(lua_object + ".animation = \"" + _game->config["spine_default_animation"].as<std::string>() + "\"");
        (*_game->lua_state).script(lua_object + ".loop_animation = true");

        animation_callback[key]();
        animation_callback[key] = (*_game->lua_state)["pass"];
    }
}

void SpineObject::setSkin(const std::string &skin) const
{
    const int resault = spSkeleton_setSkinByName(skeleton->skeleton, skin.c_str());
    spSkeleton_setSlotsToSetupPose(skeleton->skeleton);
    if (!resault)
    {
        jngl::debugLn("The Skin " + skin + " does not exist.");
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
    // Verschieben des Objektes in je die Scene
}
