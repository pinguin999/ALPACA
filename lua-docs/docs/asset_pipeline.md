# Asset Pipeline

ALPACA has a complete automated asset pipeline. Every project has a data-src folder with the source asset files like ".spine" files and a data folder for runtime optimised files.
The content of the data folder is generated automatically via the prepare_assets.py script.

> ⚠️ **All changes have to be made in the data-src folder. Otherwise changes will be overwritten.**

The data-src folder has to have the following structure:

```bash
/data-src/
    ├── audio                 # All audio as .ogg files
    ├── config
    │   └── game.json         # Game config file
    ├── dialog
    │   └── dialogs.schnack   # Schnack file
    ├── fonts                 # Fonts in .ttf format
    ├── icons                 # Game icons (512 x 512 WebP file)
    ├── scenes                # All scene definition files
    └── scripts               # All Lua scripts
```
