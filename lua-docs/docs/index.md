# Getting started with your first ALPACA game

[ALPACA on GitHub](https://github.com/pinguin999/ALPACA)

[ALPACA on Discord](https://discord.gg/zWdnq6UJ79)

If you want to use ALPACA, you have two different options:

1. You can use ALPACA for game development without compiling C++ code,
   just using Lua scripting and the Spine editor (Windows only).
2. You can set up ALPACA for engine development, where you compile the engine (C++ code)
   yourself and are able to make changes to the core functionality of ALPACA
   (this is required on Mac and Linux, but also possible on Windows).

## Setting up your PC for game development (Windows)

This is the easiest way to work with ALPACA. You don't have to deal with C++ compilers
and use a precompiled version of ALPACA. Unfortunately this only works on Windows.

1. Get [ALPACA for Windows](https://alpaca-engine.de/alpaca_engine.zip).
2. Install [Spine](https://esotericsoftware.com/) into the default folder.
3. Run `prepare_assets.exe` to synchronize your changes in the data-src folder with the data folder.
4. Run `pac.exe` to start the game.
5. In the Schnack folder you will find the dialog editor.
6. For Lua scripting it's recommended to use [VS Code](https://code.visualstudio.com/) with a Lua extension like [LuaHelper](https://marketplace.visualstudio.com/items?itemName=yinfei.luahelper).
7. [Take a look at what's in the demo and build your own game.](#whats-in-the-demo-game)
8. If you need help setting up your first project or want to talk about your game.
Book a time slot with the ALPACA developers at [calendly](https://calendly.com/pinguin999/pac-indie-game-development) 🇩🇪🇬🇧, mail me at [kolja@portaldogs.com](mailto:kolja@portaldogs.com) 🇩🇪🇬🇧 or join [ALPACA on Discord](https://discord.gg/zWdnq6UJ79).

## Setting up your PC for engine development (Windows / Mac / Linux)

This is the advanced way of working with ALPACA. You compile the engine yourself.
This also allows you to make changes to the engine core if you feel this is necessary for your use case.

1. Clone the ALPACA repository with git via `git clone --recurse https://github.com/pinguin999/ALPACA.git` or clone it with the GutHub desktop client.
2. Set up your Mac or Linux for [JNGL development](https://github.com/jhasse/jngl). Or [Windows development](https://github.com/jhasse/jngl-starter)
3. You should now be able to play the demo project.
4. Check that you have [Spine](http://de.esotericsoftware.com/), [Rhubarb](https://github.com/DanielSWolf/rhubarb-lip-sync/releases), [Lua](https://github.com/rjpcomputing/luaforwindows) and [Python](https://www.python.org/downloads/) installed. Now you can re-export all the assets from data-src to data using the prepare_assets.py script. **On Windows you will need to have [Spine](http://de.esotericsoftware.com/) installed and you can use prepare_assets.exe to export the assets.**
5. If you need help setting up your first project or want to talk about your game.
Book a time slot with the ALPACA developers at [calendly](https://calendly.com/pinguin999/pac-indie-game-development) 🇩🇪🇬🇧, mail me at [kolja@portaldogs.com](mailto:kolja@portaldogs.com) 🇩🇪🇬🇧 or join [ALPACA on Discord](https://discord.gg/zWdnq6UJ79)

## What's in the demo game?

The demo has two scenes. In the first scene there is a banana that has a funny effect when you click on it.
![Demo project](test_chamber_one.gif)
In the second scene there is an alpaca, so you have something to experiment with. If you click on the alpaca's head, it'll talk to you.
![Demo project](test_chamber_two.gif)

### What you can learn from the demo

The demo is a good place to start a new project. It already has a working setup and
some good defaults set up for you. You can also start with an empty data-src folder, but for a
smooth start it's recommended to start from the demo game.

You can play with the project and learn from it. Here are some tasks you can try yourself.

**All the game content is in the `data-src` folder and the `prepare_assets` converts it and puts it in the `data` folder. So never change any files in the `data` folder. It should always be safe to delete the `data` folder and `prepare_assets` will recreate everything from `data-src`.**

#### Adding a Spine object

All objects in an ALPACA game are Spine projects. To add a new object to a scene you need to start a new Spine project.

1. Create a folder in `data-src` with the name of your object. Let's call it `dog`
2. Open Spine and click `New Project`
3. Give the skeleton the same name as your folder. In our case it's `dog` again
4. Add images to your spine file and create some nice animations.
5. Save the Spine file to `data-src/dog/dog.spine`
6. The `prepare_assets` script should already be running, if not, run it. Check the output for `...dog.spine has been modified`
7. Now open `data-src/scenes/test_chamber_one.json` in VS Code and add the dog item to the `items` list.
8. The scene should now reload and show the dog.

```josn
{
    "items": [
        {
            "spine": "banana",
            "x": "330.0",
            "y": "400.0",
            "scale": "0.3",
            "skin": "normal"
        },
        {
            "spine": "dog",
            "x": "0.0",
            "y": "0.0"
        }
    ],
...
}
```

Find out more about [scene objects](scene.md) and [Spine](spine.md)

#### Moving an object

The dog object you have just created will be placed in the center of the screen.

1. In the game, open edit mode by pressing <kbd>Tab</kbd>
2. A green circle will appear under the dog
![Alt text](move-item.gif)
3. Drag and drop the dog where you want it.
4. Press <kbd>S</kbd> to save.

#### Playing with the scripts

Interactable regions are defined by a `bounding box` in Spine.

1. Select a `bone` in Spine and select `Bounding Box` from the new drop down menu.
2. Give the box a descriptive name. The name should represent the action. Let's name it `bark`
3. The prepare_assets script will automatically create a file called `bark.lua` for you. This file will contain the following line

```lua
print("bark")
```

4. The dog will be automatically reloaded, the clickable area can be clicked.
5. If you are not sure where the area is, you can visualise it by pressing <kbd>F10</kbd>.
6. After clicking on the area, the debug console will display the text `bark`.
7. If your dog has a bark animation, you can start the animation through the script by calling the [PlayAnimation](https://alpaca-engine.de/lua/#PlayAnimation) function.

```lua
print("bark")
PlayAnimation(0, "bark", false)
```

Now you are ready to start scripting in **Lua**, the way logic is defined in an ALPACA game. A full list of functions can be found in the [Lua API](lua.md) documentation. For the quick intro you want the player to go to the item and interact with it. To be able to go anywhere you need to define a [Point](http://esotericsoftware.com/spine-points) in Spine, so let's add the point to our item and save it. *I named the point "game center" in the example and will use this name in the script of this tutorial*.

Most Lua functions like `GoToPoint` have two versions: GoToPoint and GoToPointOn.
The GoToPoint is applied to the object calling the function.
The On version like GoToPointOn allows you to specify another object as the target.
The following example is called from the banana object, so we can use the
`GoToPoint("center")` instead of the longer version of `GoToPointOn("banana", "center")`.

```lua
print("banana_clicked")

GoToPoint("center", function ()
    PlayAnimationOn("Player", 0, "death", false, function respawn()
        print("respawn")
        PlayAnimationOn("Player", 0, "idle", true)
    end)
end)
```

A complete overview of all Lua functions can be found in the [Lua documentation](lua.md).

#### Dialog editing and audio playback

ALPACA comes with the [Schnack](https://pac4.gitlab.io/schnack-website/) node based dialog editor. The Alpaca in the demo has one dialog. You can start Schnack and open the dialog file from `data-src/dialog/dialogs.schnack`

#### Changing the game config file

First you should have a look at the file  **data-src/config/game.json**. There you can change all important basic parameters like the game name and the **start_scene**.

#### Adding a new scene

Everything in an ALPACA project is organized into **scenes**. A scene consists of a background, some items placed in the scene and music that is played in the background. Let's have a look into the  **scenes** folder. There are two example scenes. Let's copy one of the example scene files and rename it to whatever you set **start_scene** to in **game.json**. If you don't need the test_chambers it's safe to delete them.

In ALPACA everything you see on the screen is a Spine project, so for the **scene background** you need to create a Spine project. Open Spine, click on *New Project* and name the [skeleton](http://esotericsoftware.com/spine-skeletons#Skeletons) your scene name. The background will be drawn from the center, so the Spine 0,0 point should be in the center of the screen. If you want the player to be able to walk around the scene, you need a [bounding box](http://esotericsoftware.com/spine-bounding-boxes) called `walkable_area`. For reference you can look at **data-src/scene1/scene1.spine**. After saving our scene background you should update the background entry in our scene file. If it's not already running, you should run **prepare_assets.py** to get the exported files into the data folder. If you start your game now, you should see your new scene.
