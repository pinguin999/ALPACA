# Scenes

An ALPACA game exists off at least one scene. A scene (Other engines may name this room or view) is the container for everything you have on screen.

All information is defined in a .json file. Here is our example `test_chamber_one.json`.

```json
{
    "items": [
        {"spine": "banana", "x":"330.0", "y":"400.0", "scale": 0.3, "skin": "normal"},
    ],
    "backgroundMusic": "audio/ingame-action.ogg",
    "background": {"spine": "scene1"},
    "zBufferMap": null
}
```

## Items

Here we can define our items in the scene. An item always has a Spine file and a x and y position.
It can optionally have an id, scale , skin, layer, animation, cross_scene.

### spine

The name of the Spine file. The file must exist in the data folder in a subfolder with the same name.
Example: `"spine": "banana"` accepts an exported spine file at `data/banana/banana.json`.

### x

The x position of the item. The 0;0 point is in the center of the screen.
**EditMode** In a debug build of ALPACA you can press <kbd>Tab</kbd> to go into the edit mode. There you can move items around and see their new location.

![Move Item](move-item.gif)

### y

The y position of the item. For more information see x.

### id

*Optional* The id is needed to refer to an item. Per default the spine file name is used as id. But if we have more instances of an item in a scene we have to set id to refer to the items.

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

*Optional* The normal draw order is based on the y position of an item. With layers we can bring items on top of others. Default layer is 1.

### animation

*Optional* Items play the default Spine animation `animation`. If you want another animation you can define it here. The animation will be played in a loop.

### cross_scene

*Optional* Items are loaded with the scene and are destroyed when leaving the scene. If you want an item that moves to other scenes as well that you can set cross_scene to true.

## BackgroundMusic

We support one background music song per scene. If we move to another scene the music will stop and play this scene music. If boath scenes have the same background music, the music will continue playing and does not start from the beginning.

Example:

```json
    "backgroundMusic": "audio/ingame-action.ogg",
```

## Background

The background of a scene is also a Spine file. It has the parameters like an item.

To define the area where the player can walk, add a bounding box with the name `walkable_area` to the Spine file.
