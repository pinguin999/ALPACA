# Spine

[Spine](https://esotericsoftware.com/) is the core for all visuals in an ALPACA game.

ALPACA supports Spine's **Animations**, **Skins**, **Events** and **Bounding Boxes**.

## Starting a new Spine project

It's recommended to have one folder per Spine project.
**The folder name and the .spine file must have the same name. For example door/door.spine for a door object. The Spine skeleton must also be renamed to door.**

> ⚠️ **Spine object names can only contain ASCII characters, no numbers and none of these characters + - * / % ^ # < > = ( ) { } [ ] ; : , .**

### Root Position

For interactable objects the root y-position determines the rendering order. Objects with larger y-position values are rendered first. So normally you want to have the root position at the bottom of the object.

To have an object that is always in front of or behind the player, the object can be moved up or down in the spine file.

The background is always rendered at 0;0 (the center of the screen). So it's important that the root is also in the center of the spine file.

## Animations

All objects play a default animation in a loop, which can be defined in the scene file. If no animation is set in the scene, the default animation from `game.json` is used.

## Bounding Boxes

Every clickable interaction starts with a bounding box. The name of each bounding box is associated with a lua script. After you create a bounding box, the prepare_assets script will create a script of the same name for you.

*Tip: Name the bounding box a combination of the action and the object: take_banana*

## Events

Spine events are used to synchronize audio or scripts with animations.

### Audio Event

Add the filename of an ogg audio file to the event field.

### String Event

If the string event text ends with .lua, a script file with that name will be executed.
Otherwise the text will be interpreted as a lua one-liner.

## Skins

Skins can be used to change the appearance of an object. Skins are also used for the different side views of the player's character.
