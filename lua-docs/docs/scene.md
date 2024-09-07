# Scenes

An ALPACA game consists of at least one scene. A scene (other engines may call it a room or view) is the container for everything you have on the screen.

All the information is defined in a .json file. All scene files must be in `data-src/scenes`. Here is our example `data-src/scenes/test_chamber_one.json`.

```json
{
    "items": [
        {
            "spine": "banana",
            "x": "330.0",
            "y": "400.0",
            "scale": 0.3,
            "skin": "normal"
        }
    ],
    "backgroundMusic": "audio/ingame-action.ogg",
    "background": {
        "spine": "scene1"
    },
    "zBufferMap": "background",
    "left_border": -1500,
    "right_border": 1530,
    "top_border": -200,
    "bottom_border": 50
}
```

## Items

Here you can define our items in the scene. An item always has a Spine file and a x and y position.
It can optionally have an id, scale, skin, layer, animation, cross_scene.

### spine

The name of the Spine file. The file must exist in the data folder in a subfolder with the same name.
Example: `"spine": "banana"` will accept an exported spine file in `data/banana/banana.json`.

### x

The x position of the item. The 0;0 point is the center of the screen.
**EditMode** In a debug build of ALPACA you can press <kbd>Tab</kbd> to stich to into edit mode. There you can move items around and see their new position.

![Move Item](move-item.gif)

### y

The y-position of the item. See x for more information.

### id

*Optional* The id is needed to refer to an item. By default, the spine filename is used as the id. However if you have multiple instances of an item in a scene, you need to set the id to refer to the items.

Example:

```json
"items": [
        {"spine": "banana", "id": "banana1", "x":"330.0", "y":"400.0", "scale": 0.3, "skin": "normal"},
        {"spine": "banana", "id": "banana2", "x":"155.0", "y":"400.0", "scale": 0.3, "skin": "normal"},
    ],
```

### scale

*Optional* The scale of the item. Can also be negative.

### skin

*Optional* The Spine skin of the item. Only needed if you use skins in your Spine project.

### layer

*Optional* The normal drawing order is based on the y-position of an item. Layers allow you to place elements on top of each other. The default layer is 1.

### animation

*Optional* Items play the default Spine `animation`. If you want a different animation, you can specify it here. The animation will be played in a loop.

### cross_scene

*Optional* Items are loaded with the scene and destroyed when you leave the scene. If you want an item to move to other scenes, you can set cross_scene to true.

## BackgroundMusic

ALPACA supports one background music song per scene. When you move to another scene, the music stops and the scene music is played. If several scenes have the same background music, the music will continue to play instead of starting from the beginning.

Example:

```json
    "backgroundMusic": "audio/ingame-action.ogg",
```

## Background

The background of a scene is also a Spine file. It has the same parameters as an item.

To define the area where the player can walk, add a bounding box named `walkable_area` to the Spine file.
A bounding box named `non_walkable_area` will create a hole in the `walkable_area`.

## zBufferMap

To scale the player depending on its position in the scene, you can provide an additional image file.
The alpha value of the image will be multiplied with the scale of the player.
No alpha means 1, so the player is not scaled.
Full alpha means the player is scaled to 0.

The image must be placed next to the scene background in a folder named `zBufferMap`.

## Borders

With the 4 border values you can limit the camera. This prevents the black background from being visible.

## Doors

To connect two scenes ALPACA uses the concept of doors. Normally you want the player to go to the door and then switch scenes.
In the new scene the player should be where the door is.
Therefore ALPACA expects a Spine point near each door with the name of the next scene.
After the scene change the player is placed at the Spine point with the name of the last scene.

```lua
GoToPoint("scene_corridor_right", function ()
    LoadScene("scene_corridor_right")
end)
```
