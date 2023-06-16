# Getting started with your first ALPACA game

[ALPACA on GitHub](https://github.com/pinguin999/ALPACA)

[ALPACA on Discord](https://discord.gg/zWdnq6UJ79)

## Setup your PC

1. Clone the ALPACA repository with git via `git clone --recurse https://github.com/pinguin999/ALPACA.git` or clone it with the GutHub Desktop client.
2. Setup your Mac or Linux for [JNGL development](https://github.com/jhasse/jngl). Or [Windows development](https://github.com/jhasse/jngl-starter)
3. Now you should be able to play the demo project.
4. Check that you have installed [Spine](http://de.esotericsoftware.com/), [Rhubarb](https://github.com/DanielSWolf/rhubarb-lip-sync/releases), [Lua](https://github.com/rjpcomputing/luaforwindows) and [Python](https://www.python.org/downloads/). Now you can re-export all assets from data-src to data with the prepare_assets.py script. **On Windows you need [Spine](http://de.esotericsoftware.com/) installed and you can use prepare_assets.exe to export the assets.**
5. If you need help setting up your first project or want to talk about your game.
Book a time slot with the developers of ALPACA at [calendly](https://calendly.com/pinguin999/pac-indie-game-development) 🇩🇪🇬🇧
or mail me at [kolja@portaldogs.com](mailto:kolja@portaldogs.com) 🇩🇪🇬🇧 or join [ALPACA on Discord](https://discord.gg/zWdnq6UJ79)

## What's in the demo game?

The demo has two scenes. In the first scene is a banana and it has a funny effect if you click it.
The second scene is empty and you can experiment there.

### What can we learn from the demo game

We can play and learn in/from the project. Here are some easy tasks you can try out yourself.

- Add a Spine object
- Move an object
- Play with the scripts

### Let's start our own game

**All game content is in the data-src folder and the prepare_assets.py converts it and puts it into the data folder. So never change any files in the data folder. It should always be safe to delete the data folder and re-export everything from data-src.**

The demo project is a good starting point. You can also start with an empty data-src folder, but for a smooth start we start from the demo game here.

First we should look into the **data-src/config/game.json** file. There we can change all important base parameters like the game name and the **start_scene**.

Everything in an ALPACA project is organized in **scenes**. A scene exists out of a background, some items placed in the scene and music that will be played in the background. So let's look into the  **scenes** folder. There we have the two example scenes. Let's copy one of the example scene files and rename it to what we set **start_scene** to in **game.json**. If we do not need the test_chambers it's safe to delete them.

In ALPACA everything we can see on the screen is a Spine project, so for the **scene background** we need to create a Spine project. Open Spine and click *New Project* and name the [skeleton](http://esotericsoftware.com/spine-skeletons#Skeletons) to your scene name. The background is drawn from the center so the Spine 0,0 point should be in the middle of the screen. If you want the player to walk in the scene we need a `walkable_area` [bounding box](http://esotericsoftware.com/spine-bounding-boxes). For reference you can check out **data-src/scene1/scene1.spine**. After saving our scene background we should update the background entry in our scene file. If it's not already running we should start **prepare_assets.py** to get the files exported into the data folder. If you start your game now you should see your new scene.

But an empty scene is boring, so why not start with our first **object** now? Let's create a new spine project and it's a very good habit to rename the skeleton to what we want to call the object. *This name is used by the export and is needed to load an object*. Now save your Spine file for the first time. I always create a new folder for every Spine project and name the folder and the Spine file again with the same name that was used for the skeleton. Now we need images for the Spine project, if you have not made any images yet you can reuse the **banana.png** from **data-src/banana/** and copy it over into your Spine project folder.
Normally I place the image on top of the Spine 0,0 point, which makes it easy to place the object in the game and results in good results for the z-order. *But there can be situations where we want to do tricks with the z-order and place the image somewhere else in the Spine file*. Now we can try to place the object in our scene, therefore we open our **scene file** and add it with position 0,0 into the **items** section. If we now start our game we see the item in the center of the scene. *If we do not see the item, we may already have a save game file. Quit the game and delete the save game. Now try again.*
In the development mode we can press the <kbd>Tab</kbd> key to go into the edit mode and every object gets a green dot to move it around. The position will be printed in red on the dot and to the console window and if we are happy with the position we have to update the position in the **scene file**.

But we want to interact with the item. So jumping back to Spine and adding a collision box around our item and name it something like "click_ITEMNAME". Now save the Spine project and the file watcher in **prepare_assets.py** will re-export the project to data and also create a lua script with the bounding box name in the **data-src/scripts** folder. Per default the new created script prints the collision box name to the console. Press <kbd>R</kbd> ingame and all Spine files are reloaded without closing the game. Now you should be able to click your item and see a text in the console.

Now we come to **LUA scripting**, the way logic is defined in an ALPACA game. A full list of functions can be found at [LUA API](lua.md). But for the quick intro we want the player to go to the item and interact with it. To be able to go somewhere we have to define a [Point](http://esotericsoftware.com/spine-points) in Spine, so let's add the Point to our item and save it again. *I have named the Point in the example "game center" and will also use this name in this tutorial's script*.

Most LUA Function like `GoToPoint` have two versions: GoToPoint and GoToPointOn

```lua
print("banana_clicked")

function respawn()
    print("respawn")
    PlayAnimationOn("Player", 0, "idle", true, pass)
end

function play_death()
    PlayAnimationOn("Player", 0, "death", false, respawn)
end

GoToPointOn("banana", "center", play_death)
```

What we did not touch on in this quick intro: Fonts, the player or the pointer.
