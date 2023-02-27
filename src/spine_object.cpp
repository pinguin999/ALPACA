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
                if (std::equal(extension.rbegin(), extension.rend(), event_string.rbegin())) {
                    // run lua script from file
                }
                else
                {
                    _game->lua_state->script(event_string);
                }
            }
            //if (event_string.ends_with(".lua"))
        }
    }

    switch (type)
    {
    case SP_ANIMATION_INTERRUPT:
        break;

    case SP_ANIMATION_COMPLETE:
        if (!entry->loop)
        {
            reinterpret_cast<SpineObject *>(state->userData)->onAnimationComplete();
        }
        break;
    default:
        break;
    }
}

SpineObject::SpineObject(std::shared_ptr<Game> game, const std::string &spine_file, const std::string &id, float scale) : animation_callback((*game->lua_state)["pass"]), walk_callback((*game->lua_state)["pass"]), spine_name(spine_file), id(id), game(game)
{
    atlas = spAtlas_createFromFile((spine_file + "/" + spine_file + ".atlas").c_str(), nullptr);
    assert(atlas);
    spSkeletonJson *json = spSkeletonJson_create(atlas);
    json->scale = scale;

    while (true)
    {
        skeletonData = spSkeletonJson_readSkeletonDataFile(json, (spine_file + "/" + spine_file + ".json").c_str());
        if (!skeletonData)
        {
            jngl::debugLn("Fatal Error loading " + spine_file + ": " + json->error);
            // assert(skeletonData);
    		std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        break;
    }

    spSkeletonJson_dispose(json);

    animationStateData = spAnimationStateData_create(skeletonData);
    skeleton = std::make_unique<spine::SkeletonDrawable>(skeletonData, animationStateData);
    skeleton->state->userData = this;
    skeleton->state->listener = (spAnimationStateListener) & this->animationStateListener;
    playAnimation(0, game->config["spine_default_animation"].as<std::string>().c_str(), true, (*game->lua_state)["pass"]);
    bounds = spSkeletonBounds_create();

    skeleton->step();
}

std::optional<jngl::Vec2> SpineObject::getPoint(const std::string &point_name)
{
    auto slot = spSkeleton_findSlot(skeleton->skeleton, point_name.c_str());
    if (!slot)
    {
        return std::nullopt;
    }
    // Possible Problem: Slot and Point have to have the same name
    auto att = spSkeleton_getAttachmentForSlotName(skeleton->skeleton, point_name.c_str(), point_name.c_str());
    spPointAttachment *point = SUB_CAST(spPointAttachment, att);
    float x, y;
    spPointAttachment_computeWorldPosition(point, slot->bone, &x, &y);
    return jngl::Vec2(x, y);
}

std::vector<std::string> SpineObject::getPointNames()
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
                result.push_back(entry->attachment->name);
            }
            entry = entry->next;
        }
    }
    sort(result.begin(), result.end());
    return result;
}

void SpineObject::playAnimation(int trackIndex, const std::string &currentAnimation, bool loop, sol::function callback)
{
    this->animation_callback = callback;
    this->currentAnimation = currentAnimation;
    spAnimation* animation = spSkeletonData_findAnimation(skeleton->state->data->skeletonData, currentAnimation.c_str());
    if (animation)
    {
        spAnimationState_setAnimation(skeleton->state, trackIndex, animation, loop);
    }
    else
    {
        jngl::debugLn("\033[1;31m The animation " + currentAnimation + " is missing! \033[0m");
    }
}

void SpineObject::addAnimation(int trackIndex, const std::string& currentAnimation, bool loop, float delay, sol::function callback)
{
    this->animation_callback = callback;
    this->currentAnimation = currentAnimation;
    spAnimation* animation = spSkeletonData_findAnimation(skeleton->state->data->skeletonData, currentAnimation.c_str());
    if (animation)
    {
        spAnimationState_addAnimation(skeleton->state, trackIndex, animation, loop, delay);
    }
    else
    {
        jngl::debugLn("\033[1;31m The animation " + currentAnimation + " is missing! \033[0m");
    }
}

void SpineObject::onAnimationComplete()
{
    if (auto _game = game.lock())
    {
        animation_callback();
        animation_callback = (*_game->lua_state)["pass"];

        if (nextAnimation.empty())
        {
            return;
        }
        currentAnimation = nextAnimation;
        nextAnimation = "";
        playAnimation(0, currentAnimation, true, (*_game->lua_state)["pass"]);
    }
}

void SpineObject::setSkin(const std::string &skin)
{
    int resault = spSkeleton_setSkinByName(skeleton->skeleton, skin.c_str());
    spSkeleton_setSlotsToSetupPose(skeleton->skeleton);
    if (!resault)
    {
        jngl::debugLn("The Skin " + skin + " does not exist.");
    }
}

void SpineObject::activate()
{
    if (auto _game = game.lock())
    {
        _game->runAction(collision_script, getptr());
    }
}

double SpineObject::getZ()
{
    return position.y + layer * 2000.0;
}
