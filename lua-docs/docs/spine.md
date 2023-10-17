# Spine

[Spine](https://esotericsoftware.com/) is core for all visuals in an ALPACA game.

ALPACA supports Spine's **Animations**, **Skins**, **Events** and **Bounding Boxes**.

## Start a new Spine project

We recommend to have one folder per Spine project.
**The folder name and the .spine file have to have the same name. For example door/door.spine for a door object. Also the Spine skeleton must be renamed to door.**

> ⚠️ **Spine object names can only inclue ASCII characters and no numbers and none of this characters + - * / % ^ # < > = ( ) { } [ ] ; : , .**

### Root Position

For interactable objects the roots y position defines the rendering order. Objects that have a larger y position value will be rendered first. So normally you want to have the root position at the bottom of the Object.

To have a object that is alway in front or always behind the players character the object can be moved higher or lower in the spine file.

The background will alway be rendered at 0;0 (the center of the screen). Therefore it's important, that the root is also in the center of the spine file.

## Animations

All objects play a default animation in loop, this can be defined in the scene file. If no animation is set in the scene the default animation from `game.json` is used.

## Bounding Boxes

Every clickable interaction starts with a bounding box. Every bounding box's name is connected with a lua script. After creating a bounding box the prepare_assets script will create a script with the same name for you.

*Tipp: Name the bounding box a combination of an action and the object: take_banana*

## Events

Spine events help to sync audio or scripts to animations.

### Audio Event

Add the filename of an ogg audio file to the event field.

### String Event

If the string events text text ends with .lua a script file will be executed.
Else the text will be interpreted as a lua one liner.

## Skins

Skins can help to change the cisible of an object. Skins are also used for the different side views of the player's character.
