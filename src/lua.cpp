#include "game.hpp"
#include "interactable_object.hpp"

using LuaSpineObject = std::string;
using LuaSpineAnimation = std::string;
using LuaSpineSkin = std::string;
using LuaSpinePoint = std::string;
using LuaDialog = std::string;
using LuaScene = std::string;
using LuaAudio = std::string;
using LuaLanguage = std::string;

namespace {
std::optional<jngl::Vec2> getPointPosition(const std::shared_ptr<Game> &game, const std::string &pointName)
{
	std::optional<jngl::Vec2> position;
	// first look for the point in the "this" object,
	// if there is no such point, look in the current scene background
	std::shared_ptr<SpineObject> obj = (*game->lua_state)["this"];
	position = obj->getPoint(pointName);
	if (!position)
	{
		obj = game->player;
		position = obj->getPoint(pointName);
	}
	if (!position)
	{
		obj = game->currentScene->background;
		position = obj->getPoint(pointName);
	}
	return position;
}
} // anonymous namespace

void Game::setupLuaFunctions()
{
	/// pass is a function that does nothing.
	/// You can use it for testing or for unnecessary callbacks.
	lua_state->set_function("pass",
							[]()
							{
								// Just do nothing
							});

	/// Loads a new scene.
	///
	/// This door example expects a Spine point object near the door:
	///
	///	GoToPoint("cockpit", function ()
	///		LoadScene("cockpit")
	///	end)
	///
	lua_state->set_function("LoadScene",
							[this](const LuaScene &scene)
							{
								loadSceneWithFade(scene);
							});

	/// Prevent the player from skipping an interaction or walking somewere else
	lua_state->set_function("InterruptibleOff",
							[this]()
							{
								player->interruptible = false;
								lua_state->script("game.interruptible = false");
							});

	/// Enable interruption again
	lua_state->set_function("InterruptibleOn",
							[this]()
							{
								player->interruptible = true;
								lua_state->script("game.interruptible = true");
							});

	/// Play an animation on the calling Spine object
	/// int trackIndex: Spine animation track.
	/// string newAnimation: Name of the animation to play.
	/// bool loop: Whether to loop the animation at the end.
	/// function callback: Function to be called at the end of the animation, also for looped animations.
	lua_state->set_function("PlayAnimation",
							[this](int trackIndex, const LuaSpineAnimation &newAnimation, bool loop, std::optional<sol::function> callback)
							{
								if (!callback) {
									callback = (*lua_state)["pass"];
								}
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->playAnimation(trackIndex, newAnimation, loop, callback.value());
								const std::string lua_object = getLuaPath(obj->getId());
								lua_state->script(lua_object + ".animation = \"" + newAnimation + "\"");
								if (loop)
								{
									lua_state->script(lua_object + ".loop_animation = true");
								}
								else
								{
									lua_state->script(lua_object + ".loop_animation = false");
								}
							});

	/// Adds an animation to the calling Spine object that will be played after the current animation ends.
	/// int trackIndex: Spine animation track.
	/// string newAnimation: Name of the animation to play.
	/// bool loop: Whether to loop the animation at the end.
	/// function callback: Function to be called at the end of the animation, also for looped animations.
	lua_state->set_function("AddAnimation",
							[this](int trackIndex, const LuaSpineAnimation &newAnimation, bool loop, float delay, std::optional<sol::function> callback)
							{
								if (!callback) {
									callback = (*lua_state)["pass"];
								}
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->addAnimation(trackIndex, newAnimation, loop, delay, callback.value());
								const std::string lua_object = getLuaPath(obj->getId());
								lua_state->script(lua_object + ".animation = \"" + newAnimation + "\"");
								if (loop)
								{
									lua_state->script(lua_object + ".loop_animation = true");
								}
								else
								{
									lua_state->script(lua_object + ".loop_animation = false");
								}
							});

	/// See PlayAnimation
	/// SpineObject object: ID of the object to affect
	/// int trackIndex: Spine animation track.
	/// string newAnimation: Name of the animation to play.
	/// bool loop: Whether to loop the animation at the end.
	/// function callback: Function to be called at the end of the animation, also for looped animations.
	lua_state->set_function("PlayAnimationOn",
							[this](const LuaSpineObject &object, int trackIndex, const LuaSpineAnimation &newAnimation, bool loop, std::optional<sol::function> callback)
							{
								if (!callback) {
									callback = (*lua_state)["pass"];
								}
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->playAnimation(trackIndex, newAnimation, loop, callback.value());
									const std::string lua_object = getLuaPath(obj->getId());
									lua_state->script(lua_object + ".animation = \"" + newAnimation + "\"");
									if (loop)
									{
										lua_state->script(lua_object + ".loop_animation = true");
									}
									else
									{
										lua_state->script(lua_object + ".loop_animation = false");
									}
								}
							});

	/// See AddAnimation
	/// SpineObject object: ID of the object to affect
	/// int trackIndex: Spine animation track.
	/// string newAnimation: Name of the animation to play.
	/// bool loop: Whether to loop the animation at the end.
	/// function callback: Function to be called at the end of the animation, also for looped animations.
	lua_state->set_function("AddAnimationOn",
							[this](const LuaSpineObject &object, int trackIndex, const LuaSpineAnimation &newAnimation, bool loop, float delay, std::optional<sol::function> callback)
							{
								if (!callback) {
									callback = (*lua_state)["pass"];
								}
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->addAnimation(trackIndex, newAnimation, loop, delay, callback.value());
									const std::string lua_object = getLuaPath(obj->getId());
									lua_state->script(lua_object + ".animation = \"" + newAnimation + "\"");
									if (loop)
									{
										lua_state->script(lua_object + ".loop_animation = true");
									}
									else
									{
										lua_state->script(lua_object + ".loop_animation = false");
									}
								}
							});

	/// Set a skin on a Spine object
	/// string skin: The name of the Spine skin
	lua_state->set_function("SetSkin",
							[this](const LuaSpineSkin &skin)
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->setSkin(skin);
								const std::string lua_object = getLuaPath(obj->getId());
								lua_state->script(lua_object + ".skin = \"" + skin + "\"");
							});

	/// See SetSkin
	/// SpineObject object: ID of the object to affect
	/// string skin: The name of the Spine skin
	lua_state->set_function("SetSkinOn",
							[this](const LuaSpineObject &object, const LuaSpineSkin &skin)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->setSkin(skin);
									const std::string lua_object = getLuaPath(obj->getId());
									lua_state->script(lua_object + ".skin = \"" + skin + "\"");
								}
							});

	/// Play a dialog by name
	/// string dialogName: The dialog to play.
	lua_state->set_function("PlayDialog",
							[this](const LuaDialog &dialogName, std::optional<sol::function> callback)
							{
								if (!callback) {
									callback = (*lua_state)["pass"];
								}
								float x = 0;
								float y = 0;
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								const auto *slot = spSkeleton_findSlot(obj->skeleton->skeleton, "head");
								if (slot)
								{
									auto *att = spSkeleton_getAttachmentForSlotName((obj)->skeleton->skeleton, "head", "head");
									spPointAttachment *point = SUB_CAST(spPointAttachment, att);
									spPointAttachment_computeWorldPosition(point, slot->bone, &x, &y);
								}
								getDialogManager()->play(dialogName, jngl::Vec2(x, -y) + obj->getPosition(), callback.value());
							});

	/// Add the current item to the inventory.
	/// Sets the skin of the item of the item to inventory_default_skin
	/// Move the item from the scene to inventory_items.
	lua_state->set_function("AddToInventory",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								sol::optional<sol::lua_table> item = (*lua_state)["scenes"][(*lua_state)["game"]["scene"]]["items"][obj->getId()];
								if (!item)
								{
									return;
								}

								obj->setSkin((*lua_state)["config"]["inventory_default_skin"]);
								obj->setCrossScene(true);
								obj->setVisible(false);
								(*lua_state)["inventory_items"][obj->getId()] = item;
								(*lua_state)["inventory_items"][obj->getId()]["skin"] = (*lua_state)["config"]["inventory_default_skin"];
								(*lua_state)["inventory_items"][obj->getId()]["cross_scene"] = true;

								(*lua_state)["scenes"][(*lua_state)["game"]["scene"]]["items"][obj->getId()] = sol::lua_nil;
							});

	/// See AddToInventory
	/// string skin: The name of the Spine skin
	lua_state->set_function("AddToInventoryWithSkin",
							[this](const LuaSpineSkin &skin)
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								sol::optional<sol::lua_table> item = (*lua_state)["scenes"][(*lua_state)["game"]["scene"]]["items"][obj->getId()];
								if (!item)
								{
									return;
								}

								obj->setSkin(skin);
								obj->setCrossScene(true);
								obj->setVisible(false);
								(*lua_state)["inventory_items"][obj->getId()] = item;
								(*lua_state)["inventory_items"][obj->getId()]["skin"] = skin;
								(*lua_state)["inventory_items"][obj->getId()]["cross_scene"] = true;

								(*lua_state)["scenes"][(*lua_state)["game"]["scene"]]["items"][obj->getId()] = sol::lua_nil;
							});

	/// See AddToInventory
	/// SpineObject object: ID of the object to affect
	/// Note: The object must be in the scene, this function does not create new objects. The object can be placed outside the scene view.
	lua_state->set_function("AddToInventoryOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									sol::optional<sol::lua_table> item = (*lua_state)["scenes"][(*lua_state)["game"]["scene"]]["items"][obj->getId()];
									if (!item)
									{
										return;
									}

									obj->setSkin((*lua_state)["config"]["inventory_default_skin"]);
									obj->setCrossScene(true);
									obj->setVisible(false);
									(*lua_state)["inventory_items"][obj->getId()] = item;
									(*lua_state)["inventory_items"][obj->getId()]["skin"] = (*lua_state)["config"]["inventory_default_skin"];
									(*lua_state)["inventory_items"][obj->getId()]["cross_scene"] = true;

									(*lua_state)["scenes"][(*lua_state)["game"]["scene"]]["items"][obj->getId()] = sol::lua_nil;
								}
							});

	/// See AddToInventory
	/// SpineObject object: ID of the object to affect
	/// string skin: The name of the Spine skin
	/// Note: See the AddToInventoryOn note.
	lua_state->set_function("AddToInventoryWithSkinOn",
							[this](const LuaSpineObject &object, const LuaSpineSkin &skin)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									sol::optional<sol::lua_table> item = (*lua_state)["scenes"][(*lua_state)["game"]["scene"]]["items"][obj->getId()];
									if (!item)
									{
										return;
									}

									obj->setSkin(skin);
									obj->setCrossScene(true);
									obj->setVisible(false);
									(*lua_state)["inventory_items"][obj->getId()] = item;
									(*lua_state)["inventory_items"][obj->getId()]["skin"] = skin;
									(*lua_state)["inventory_items"][obj->getId()]["cross_scene"] = true;

									(*lua_state)["scenes"][(*lua_state)["game"]["scene"]]["items"][obj->getId()] = sol::lua_nil;
								}
							});

	/// DEPRECATED use SetDeleted
	lua_state->set_function("RemoveFromInventory",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								lua_state->script("inventory_items[\"" + obj->getId() + "\"] = nil");
								obj->setCrossScene(false);
							});

	/// DEPRECATED use SetDeleted
	lua_state->set_function("RemoveFromInventoryOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									lua_state->script("inventory_items[\"" + object + "\"] = nil");
									obj->setCrossScene(false);
								}
							});

	/// Set deleted in the current object.
	/// Objects are only deleted at the end of the frame.
	lua_state->set_function("SetDeleted",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								auto inter = std::static_pointer_cast<InteractableObject>(obj);
								obj->setParent(nullptr);
								for (auto it = pointer->attachedObjects.begin(); it != pointer->attachedObjects.end();)
								{
									if ((*it) == nullptr || (*it) == obj)
									{
										it = pointer->attachedObjects.erase(it);
									}
									else
									{
										++it;
									}
								}
								if (inter)
								{
									inter->registerToDelete();
								}
							});

	/// See SetDeleted
	/// SpineObject object: ID of the object to affect
	/// Note: SetDeleted cannot be called on the last frame of an animation via a Spine event.
	lua_state->set_function("SetDeletedOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									auto inter = std::static_pointer_cast<InteractableObject>(obj);
									obj->setParent(nullptr);
									for (auto it = pointer->attachedObjects.begin(); it != pointer->attachedObjects.end();)
									{
										if ((*it) == nullptr || (*it) == obj)
										{
											it = pointer->attachedObjects.erase(it);
										}
										else
										{
											++it;
										}
									}
									if (inter)
									{
										inter->registerToDelete();
									}
								}
							});

	/// Get all Spine points from this Spine object
	/// returns: a list of points
	lua_state->set_function("GetPointNames",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								return obj->getPointNames();
							});

	/// See GetPointNames
	/// Lua Example:
	///
	/// local points = GetPointNamesOn("inventory_object")
	/// for i = 1, #points do
	///    print(i, points[i])
	/// end
	/// SpineObject object: ID of the object to affect
	/// returns: a list of positions
	lua_state->set_function("GetPointNamesOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									return sol::as_table(obj->getPointNames());
								}
								throw std::runtime_error("No object " + object);
							});

	/// Send the player to the position of a point of this Spine object.
	/// string point_name: Name of the Spine point the player should go to
	/// function callback: Function to be called when the layer reaches the position
	lua_state->set_function("GoToPoint",
							[this](const LuaSpinePoint &point_name, std::optional<sol::function> callback)
							{
								if (!callback) {
									callback = (*lua_state)["pass"];
								}
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								auto position = obj->getPoint(point_name);
								if (!position)
								{
									jngl::error("Point " + point_name + " not found.");
									return;
								}
								player->addTargetPositionImmediately(obj->getPosition() + *position, callback.value());
								pointer->setPrimaryHandled();
								// TODO Write Players position to Lua
							});

	/// See GoToPoint
	/// SpineObject object: ID of the object to affect
	/// string point_name: Name of the Spine point the player should go to
	/// function callback: Function to be called when the layer reaches the position
	lua_state->set_function("GoToPointOn",
							[this](const LuaSpineObject &object, const LuaSpinePoint &point_name, std::optional<sol::function> callback)
							{
								if (!callback) {
									callback = (*lua_state)["pass"];
								}
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									auto position = obj->getPoint(point_name);
									if (!position)
									{
										jngl::error("Point " + point_name + " not found.");
										return;
									}
									player->addTargetPositionImmediately(obj->getPosition() + *position, callback.value());
									pointer->setPrimaryHandled();
									// TODO Write Players position to Lua
								}
							});

	/// Stop the player at this position.
	lua_state->set_function("StopWalking",
							[this]()
							{
								player->stop_walking();
							});

	/// Debug function to get the position of a Spine point.
	/// string point_name: Name of the Spine point
	/// returns: Tupe(x, y)
	lua_state->set_function("GetPointPosition",
							[this](const LuaSpinePoint &point_name)
							{
								auto position = getPointPosition(shared_from_this(), point_name);
								if (position)
								{
									return std::tuple(position->x, position->y);
								}

								return std::tuple(0.0, 0.0);
							});

	/// Set the position of an object to a Spine point.
	/// string point_name: Name of the Spine point
	lua_state->set_function("SetPositionToPoint",
							[this](const LuaSpinePoint &point_name)
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								auto position = getPointPosition(shared_from_this(), point_name);

								if (position)
								{
									const std::shared_ptr<Player> point = std::dynamic_pointer_cast<Player>(obj);
									if (point)
									{
										point->setPosition(position.value());
										point->addTargetPositionImmediately(position.value(), (*lua_state)["pass"]);
									}
									obj->setPosition(position.value());
									const std::string lua_object = getLuaPath(obj->getId());
									lua_state->script(lua_object + ".x = " + std::to_string(position->x) + "");
									lua_state->script(lua_object + ".y = " + std::to_string(position->y) + "");
								}
							});

	/// See SetPositionToPoint
	/// SpineObject object: ID of the object to affect
	/// string point_name: Name of the Spine point
	lua_state->set_function("SetPositionToPointOn",
							[this](const LuaSpineObject &object, const LuaSpinePoint &point_name)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									const std::shared_ptr<SpineObject> self = (*lua_state)["this"];
									auto position = getPointPosition(shared_from_this(), point_name);
									if (position)
									{
										const std::shared_ptr<Player> point = std::dynamic_pointer_cast<Player>(obj);
										if ( point != nullptr)
										{
											point->setPosition(position.value());
											point->stop_walking();
											point->addTargetPositionImmediately(position.value(), (*lua_state)["pass"]);
										}
										obj->setPosition(position.value());
										const std::string lua_object = getLuaPath(obj->getId());
										lua_state->script(lua_object + ".x = " + std::to_string(position->x) + "");
										lua_state->script(lua_object + ".y = " + std::to_string(position->y) + "");
									}
								}
							});

	// See SetPositionToPoint
	/// SpineObject object: ID of the object to affect
	/// string from: Objects ID that provides the point
	/// string point_name: Name of the Spine point
	lua_state->set_function("SetOnToPointFrom",
							[this](const LuaSpineObject &object, const LuaSpineObject &from, const LuaSpinePoint &point_name)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								const std::shared_ptr<SpineObject> frm = getObjectById(from);
								if (frm)
								{
									auto position = frm->getPoint(point_name);
									if (position && frm->abs_position)
									{
										position = position.value() + getCameraPosition();
									}

									if (obj && position)
									{
										obj->setPosition(frm->getPosition() + position.value());
										const std::string lua_object = getLuaPath(obj->getId());
										lua_state->script(lua_object + ".x = " + std::to_string(frm->getPosition().x + position->x) + "");
										lua_state->script(lua_object + ".y = " + std::to_string(frm->getPosition().y + position->y) + "");
									}
								}
							});

	/// Hide a Spine object.
	lua_state->set_function("SetHidden",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->setVisible(false);
								const std::string lua_object = getLuaPath(obj->getId());
								lua_state->script(lua_object + ".visible = false");
							});

	/// See SetHidden
	/// SpineObject object: ID of the object to affect
	lua_state->set_function("SetHiddenOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->setVisible(false);
									const std::string lua_object = getLuaPath(obj->getId());
									lua_state->script(lua_object + ".visible = false");
								}
							});

	/// Make a Spine object visible.
	lua_state->set_function("SetVisible",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->setVisible(true);
								const std::string lua_object = getLuaPath(obj->getId());
								lua_state->script(lua_object + ".visible = true");
							});

	/// See SetVisible
	/// SpineObject object: ID of the object to affect
	lua_state->set_function("SetVisibleOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->setVisible(true);
									const std::string lua_object = getLuaPath(obj->getId());
									lua_state->script(lua_object + ".visible = true");
								}
							});

	/// Set the render layer of an Object.
	/// It's also possible to make objects non-interactive below a certain layer.
	/// int layer: Number of the layer
	lua_state->set_function("SetLayer",
							[this](int layer)
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->layer = layer;
								const std::string lua_object = getLuaPath(obj->getId());
								if (!lua_object.empty())
								{
									lua_state->script(lua_object + ".layer = " + std::to_string(layer));
								}
							});

	/// See SetLayer
	/// SpineObject object: ID of the object to affect
	lua_state->set_function("SetLayerOn",
							[this](const LuaSpineObject &object, int layer)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->layer = layer;
									const std::string lua_object = getLuaPath(obj->getId());
									if (!lua_object.empty())
									{
										lua_state->script(lua_object + ".layer = " + std::to_string(layer));
									}
								}
							});

	/// Make all objects with a layer below this value non-interactive. Useful for a menu, inventory or intro that plays over a scene.
	/// int layer: The layer number [default: 0].
	lua_state->set_function("SetInactivLayerBorder",
							[this](int layer)
							{
								setInactivLayerBorder(layer);
							});

	/// Attach the Spine object to the mouse pointer.
	lua_state->set_function("AttachToPointer",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->setParent(pointer);
								pointer->attachedObjects.push_back(obj);
							});

	/// See AttachToPointer
	/// SpineObject object: ID of the object to affect
	lua_state->set_function("AttachToPointerOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->setParent(pointer);
									pointer->attachedObjects.push_back(obj);
								}
							});

	/// Deatatch all Spine object from the pointer
	lua_state->set_function("DeattachAllFromPointer",
							[this]()
							{
								for (auto &obj : pointer->attachedObjects)
								{
									obj->setParent(nullptr);
									obj->setVisible(false);
								}
								pointer->attachedObjects.clear();
							});

	/// Deatatch the Spine object from the pointer
	lua_state->set_function("DeattachFromPointer",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->setParent(nullptr);

								for (auto it = pointer->attachedObjects.begin(); it != pointer->attachedObjects.end();)
								{
									if ((*it) == nullptr || (*it) == obj)
									{
										it = pointer->attachedObjects.erase(it);
									}
									else
									{
										++it;
									}
								}
							});

	/// See DeattachFromPointer
	/// SpineObject object: ID of the object to affect
	lua_state->set_function("DeattachFromPointerOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->setParent(nullptr);

									for (auto it = pointer->attachedObjects.begin(); it != pointer->attachedObjects.end();)
									{
										if ((*it) == nullptr || (*it) == obj)
										{
											it = pointer->attachedObjects.erase(it);
										}
										else
										{
											++it;
										}
									}
								}
							});

	/// Is the Spine object attached to the mouse pointer
	/// returns: bool
	lua_state->set_function("IsAttachedToPointer",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								auto parent = obj->getParent();
								return parent != nullptr;
							});

	/// See IsAttachedToPointer
	/// SpineObject object: ID of the object to affect
	/// returns: bool
	lua_state->set_function("IsAttachedToPointerOn",
							[this](const LuaSpineObject &object)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									auto parent = obj->getParent();
									return parent != nullptr;
								}
								return false;
							});

	/// Is something attached to the mouse pointer
	/// returns: bool
	lua_state->set_function("IsSomethingAttachedToPointer",
							[this]()
							{
								return !pointer->attachedObjects.empty();
							});

	/// Rote a Spine object
	/// float rotation: A number between 0.0 and 365.0
	lua_state->set_function("SetRotation",
							[this](const float rotation)
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->setRotation(rotation);
							});

	/// See SetRotation
	/// SpineObject object: ID of the object to affect
	/// float rotation: A number between 0.0 and 365.0
	lua_state->set_function("SetRotationOn",
							[this](const LuaSpineObject &object, const float rotation)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->setRotation(rotation);
								}
							});

	/// Scale a Spine object
	/// float: The scale
	lua_state->set_function("SetScale",
							[this](const float scale)
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								obj->setScale(scale);
								const std::string lua_object = getLuaPath(obj->getId());
								lua_state->script(lua_object + ".scale = \"" + std::to_string(scale) + "\"");
							});

	/// See SetScale
	/// SpineObject object: ID of the object to affect
	/// float: The scale
	lua_state->set_function("SetScaleOn",
							[this](const LuaSpineObject &object, const float scale)
							{
								const std::shared_ptr<SpineObject> obj = getObjectById(object);
								if (obj)
								{
									obj->setScale(scale);
									const std::string lua_object = getLuaPath(obj->getId());
									lua_state->script(lua_object + ".scale = \"" + std::to_string(scale) + "\"");
								}
							});

	/// Returns the time in seconds since the game started.
	/// returns: Number
	lua_state->set_function("GetTime",
							[]()
							{
								return jngl::getTime();
							});

	/// Set the maximum speed of the player.
	/// Can be set to 0 if the player should not be able to walk.
	/// float speed: value of the maximum speed
	lua_state->set_function("SetPlayerMaxSpeed",
							[this](float max_speed)
							{
								player->setMaxSpeed(max_speed);
								(*lua_state)["scenes"]["cross_scene"]["items"]["player"]["max_speed"] = max_speed;
							});

	/// Create a game object from a Spine file
	/// string spine_file
	/// string id
	/// float scale
	lua_state->set_function("CreateObject",
							[this](const std::string &spine_file, const  std::string &id, float scale)
							{
								auto interactable = currentScene->createObject(spine_file, id, scale);
								interactable->toLuaState();

								add(std::static_pointer_cast<SpineObject>(interactable));
							});

	/// Playing an audio file.
	/// It's much better to use Spine events to trigger the sound to keep it in sync with the animation
	/// string file: The audio file
	lua_state->set_function("PlayAudio",
							[](const LuaAudio &file)
							{
								jngl::play("audio/" + file);
							});

	/// Stop an audio file.
	/// string file: The audio file
	lua_state->set_function("StopAudio",
							[](const LuaAudio &file)
							{
								jngl::stop("audio/" + file);
							});

	/// Checks if an audio file is playing.
	/// string file: The audio file
	/// returns:a bool indication if the audio is playing
	lua_state->set_function("isAudioPlaying",
							[](const LuaAudio &file)
							{
								return jngl::isPlaying("audio/" + file);
							});

	/// Get the objects ID
	/// returns: the name as string
	lua_state->set_function("GetID",
							[this]()
							{
								const std::shared_ptr<SpineObject> obj = (*lua_state)["this"];
								return obj->getId();
							});

	/// Set the player's x-scale
	/// float: The scale
	lua_state->set_function("SetPlayerScaleX",
							[this](const float scale)
							{
								player->skeleton->skeleton->scaleX = scale;
							});

	/// Set the language
	/// string: Example 'de' or 'en'
	lua_state->set_function("SetLanguage",
							[this](const LuaLanguage &language)
							{
								const std::vector<std::string> languages =  (*lua_state)["config"]["supportedLanguages"].get<std::vector<std::string>>();
								for (const auto &supported_language : languages)
								{
									if (language == supported_language)
									{
										this->language = language;
										const std::string dialogFilePath = (*lua_state)["config"]["dialog"];
										getDialogManager()->loadDialogsFromFile(dialogFilePath, false);
										return;
									}
								}
							});

	/// Exit the game
	/// Not supported on iOS
	lua_state->set_function("Exit",
							[]()
							{
								jngl::quit();
							});

	/// Write a savegame
	lua_state->set_function("SaveGame",
							[this]()
							{
								saveLuaState();
							});

	/// Load the savegame
	lua_state->set_function("LoadGame",
							[this]()
							{
								gameObjects.clear();
								lua_state = {};
								currentScene = nullptr;
								player = nullptr;
								pointer = nullptr;
								lua_state = std::make_shared<sol::state>();
								lua_state->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math);
								init();
							});

	/// Delete the savegame file
	lua_state->set_function("DeleteSaveGame",
							[]()
							{
								jngl::writeConfig("savegame", "");
							});

	/// Set the zBufferMap to a file
	lua_state->set_function("SetzBufferMap",
							[this](const std::string &file)
							{
								currentScene->zBufferMap = jngl::ImageData::load(file);
#ifndef NDEBUG
								currentScene->background->sprite = std::make_unique<jngl::Sprite>(*currentScene->zBufferMap, jngl::getScaleFactor());
#endif
								(*lua_state)["scenes"][currentScene->getSceneName()]["zBufferMap"] = file;
							});

	/// Remove the zBufferMap
	lua_state->set_function("RemovezBufferMap",
							[this]()
							{
								currentScene->zBufferMap = nullptr;
#ifndef NDEBUG
								currentScene->background->sprite = nullptr;
#endif
								if ((*lua_state)["scenes"][currentScene->getSceneName()]["zBufferMap"].valid())
								{
									(*lua_state)["scenes"][currentScene->getSceneName()]["zBufferMap"] = sol::lua_nil;
								}
							});
}
