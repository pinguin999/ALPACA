# Asset Pipeline

ALPACA has a complete automated asset pipeline. Every project has a data folder with for runtime optimised files and a data-src folder with the source asset files like .spine files. The content of the data folder is generated automatically via the prepare_assets.py script.

**All changes should be made in the data-src folder. Otherwise changes will be overwritten**

The data-src folder should have the following structure:

```bash
/data-src/
    ├── audio
    ├── config
    │   └── game.json         # Game config file
    ├── dialog
    │   └── dialogs.schnack   # Schnack file
    ├── fonts                 # Font in .ttf format
    ├── icons                 # Game icons (512 x 512 WebP file)
    ├── scenes                # All scene definition files
    └── scripts               # All LUA scripts.
```
