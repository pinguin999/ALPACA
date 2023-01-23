# ALPACA

ALPACA (short for "A Library for Point And Click Adventures") is a game engine for adventure games and interactive comics.
It's written in C++ and can easily be scripted in Lua.

## Features

- Easy to use 🐣
- Gif Recording 🎉
- Node based dialog editor 🗯️
- Integrated dialog engine 💬
- PDF export for voice actors 🎤
- 2D bone animations via Spine 🦴💀
- Events via animation 🦾
- True multi-platform via JNGL 💻📱📺🖥️
- Hot reload 🔥
- Mouse, controller and touch support 🎮
- A* navigation 🧭
- Easy scripting via Lua 📇
- Automatic save game handling 💾
- Automatic tests playing the game 🤖
- Build via GitLab CI 👷‍♂️

## These tools make ALPACA possible

- [Spine](http://esotericsoftware.com/) is used for all graphic assets and animations.
- [JNGL](https://github.com/jhasse/jngl) is the cross platform game library that makes it possible to publish to Nintendo Switch, Xbox, PC, Mac, iOS, Android and the Web.
- [Schnack](https://gitlab.com/pac4/schnack) is a node based dialog editor.
- [Rhubarb](https://github.com/DanielSWolf/rhubarb-lip-sync) for creation of lip sync information from audio files.
- [Lua](https://www.lua.org/) is the scripting language used for game logic.

All these tools are managed by a Python asset pipeline with file watchers for fast reloading of changes into the game.

## Links and examples

![Demo project](test_chamber_one.gif)
Scene one of demo project

[Project setup and Lua interface documentation](https://alpaca-engine.de/)

[Example on the web](https://alpaca-engine.de/demo/)

## Getting Started

Everything in ALPACA is organized in scenes. And we already provide two demo scenes, you can easily modify. The scene is defined in the data-src/scenes/test_chamber_one.json file.

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

In this demo scene you have a background and one item. You can change the x and y coordinates of the item or duplicate it.

If you want to **add a new item**, you first have to create a new Spine project and save it in date-src. More details in the [docs](https://alpaca-engine.de/).

If you want to **create a third scene**, you just have to create a new file or duplicate the test_chamber_one.json and change it to your needs.

## Build ALPACA from source

### Linux und macOS

```bash
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Windows

Install [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/) and please make sure to select **Desktop development with C++** while installing.

Then you only need to choose **Open a local folder** and open the ALPACA folder.
Visual Studio will automatically run CMake and you can choose pac.exe as target and press F5 to build and start the game.

## Contact

If you need help setting up your first project or want to talk about your game.
Book a time slot with the developers of ALPACA at [calendly](https://calendly.com/pinguin999/pac-indie-game-development) 🇩🇪🇬🇧
or mail me at [kolja@portaldogs.com](kolja@portaldogs.com) 🇩🇪🇬🇧

## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License

- Code distributed under the [zlib License](https://choosealicense.com/licenses/zlib/)
- Assets (except Spine Boy) distributed under [CC-BY](https://creativecommons.org/licenses/by/4.0/)
