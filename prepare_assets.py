# How to use
# ==========
# pip install pipenv
# pipenv install -d
# pipenv run python prepare_assets.py

# Quelle https://thepythoncorner.com/posts/2019-01-13-how-to-create-a-watchdog-in-python-to-look-for-filesystem-changes/
# We need python 3.10 for cross platform newline

# Export to exe on windows via
# pyinstaller --onefile .\prepare_assets.py

from __future__ import annotations

import contextlib
import hashlib
import json
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, field
from multiprocessing import Pool, freeze_support
from os import cpu_count, walk
from pathlib import Path
from stat import S_IREAD, S_IRGRP, S_IROTH, S_IWUSR
from typing import Any, Literal, TypedDict
import pickle

import requests
from termcolor import colored
from watchdog.events import PatternMatchingEventHandler
from watchdog.observers import Observer
from collections import defaultdict


class FileErrors(TypedDict):
    file: str
    errors: list[str]


SCHNACKER_FOLDER = "data/dialog/"
RHUBARB_OUT = "data/rhubarb/"

SPINE_THREADS = cpu_count()

set_read_only = True
# could be "linux", "linux2", "linux3", ...
if sys.platform.startswith("linux"):
    RHUBARB = "/usr/bin/rhubarb"
    SPINE = "/usr/bin/spine"
    LUA = "luac"
elif sys.platform == "darwin":
    RHUBARB = "/Applications/Rhubarb-Lip-Sync-1.14.0-macOS/rhubarb"
    SPINE = "/Applications/Spine.app/Contents/MacOS/Spine"
    LUA = "luac"
elif sys.platform == "win32":
    # Windows
    SPINE = "C:\\Program Files\\Spine\\Spine.exe"
    if getattr(sys, "frozen", False):
        mod_path = Path(sys.executable).resolve().parent
    elif __file__:
        mod_path = Path(__file__).parent
    RHUBARB = (mod_path / "windows_bin\\rhubarb.exe").resolve()
    LUA = (mod_path / "windows_bin\\luac.exe").resolve()
    set_read_only = False


spine_objects: dict[str, set[str]] = {
    "Player": {"internal"},
    "Background": {"internal"},
}
spine_animations: dict[str, set[str]] = {}
spine_skins: dict[str, set[str]] = {}
spine_points: dict[str, set[str]] = {}
all_dialogs: dict[str, set[str]] = {}
all_scenes: dict[str, set[str]] = {}
all_audio: dict[str, set[str]] = {}
all_language: dict[str, set[str]] = {}
schnack_vars: dict[str, str] = {}
schnack_characters_props: dict[str, str] = {}

scene_files = []  # Temp list to prevent infinite loop


class Progress:
    def __init__(
        self: Progress,
        initialCount: int,
        maxCount: int,
        maxTitleLength: int = 26,
        barLength: int = 46,
    ) -> None:
        self._initialCount = initialCount
        self._currentCount = initialCount
        self._maxCount = maxCount
        self._title = ""
        self._maxTitleLength = maxTitleLength
        self._barLength = barLength
        self._errors: list[str] = []

    def updateTitle(self: Progress, title: str) -> None:
        self._title = title
        self.print_status()

    def advance(self: Progress, count: int = 1) -> None:
        self._currentCount += count
        self.print_status()

    def print_status(self: Progress) -> None:
        if self._maxCount == 0:
            return
        percent = float(self._currentCount) / self._maxCount
        printedTitle = (
            self._title
            if len(self._title) <= self._maxTitleLength
            else "[...]" + self._title[len(self._title) - self._maxTitleLength - 5 :]
        )

        filledCount = int(self._barLength * percent)
        filled = "█" * filledCount
        unfilled = "-" * (self._barLength - filledCount)
        percentage = "%3.1f" % (percent * 100)
        print(
            colored(
                f"\r{printedTitle} |{filled}{unfilled}| {percentage}%",
                "red" if len(self._errors) > 0 else "blue",
            ),
            end="\r",
        )

    def finish(self: Progress) -> None:
        print("\n")

    def addError(self: Progress, errorMsg: str) -> None:
        self._errors.append(errorMsg)
        self.print_status()


def rhubarb_export(
    node_info: tuple[str, str, dict[str, str]],
) -> tuple[list[Any], dict[str, str]]:
    node_id = node_info[0]
    db: dict[str, str] = node_info[2]
    file = f"data/audio/{node_id}.ogg"
    errors: list[str] = []

    db_out: dict[str, str] = {}

    with open(file, "rb") as f:
        checksum = hashlib.file_digest(f, "sha256").hexdigest()
        db_out[file] = checksum
        if db.get(file, None) == checksum:
            return errors, db_out

    if not RHUBARB:
        return errors, {}
    rhubarb_out = Path(f"{RHUBARB_OUT}{node_id}.json")
    with contextlib.suppress(FileNotFoundError):
        if set_read_only:
            rhubarb_out.chmod(S_IREAD | S_IRGRP | S_IROTH | S_IWUSR)
    command = [RHUBARB, file, "-r", "phonetic", "-f", "json", "-o", str(rhubarb_out)]
    p = subprocess.Popen(
        command, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    output = p.communicate()[0]
    if p.returncode != 0:
        errors.append(output.decode())
    with contextlib.suppress(FileNotFoundError):
        if set_read_only:
            rhubarb_out.chmod(S_IREAD | S_IRGRP | S_IROTH)
    return errors, db_out


def rhubarb_reexport() -> None:
    global db
    nodes = get_notes()

    progress = Progress(0, len(nodes))
    progress.updateTitle(" Re-Exporting Rhubarb Files")
    results = []
    Path(RHUBARB_OUT).mkdir(parents=True, exist_ok=True)
    with Pool(SPINE_THREADS) as p:
        for i, (errors, db_out) in enumerate(
            p.imap_unordered(rhubarb_export, nodes), 0
        ):
            progress.advance()

            if len(errors) > 0:
                results.append(
                    {
                        "file": nodes[i],
                        "errors": errors,
                    }
                )
            db = db | db_out
    progress.finish()


def apply_rhubarb(character_in: str | None = None) -> None:
    nodes = get_notes(character_in)
    # Group nodes by character_name
    nodes_by_character = defaultdict(list)
    for node_info in nodes:
        node_id = node_info[0]
        character_name = node_info[1]
        nodes_by_character[character_name].append(node_id)

    for character_name, node_ids in nodes_by_character.items():
        character_path = Path(f"data/{character_name}/{character_name}.json")
        if not character_path.exists():
            print("Can not write into: ", character_path)
            continue
        if set_read_only:
            character_path.chmod(S_IREAD | S_IRGRP | S_IROTH | S_IWUSR)
        with character_path.open("r+") as character_file:
            character = json.load(character_file)
            if "animations" not in character:
                character["animations"] = {}

            for node_id in node_ids:
                rhubarb_out = Path(f"{RHUBARB_OUT}{node_id}.json")
                if not rhubarb_out.exists():
                    continue
                with rhubarb_out.open() as rhubarb_outfile:
                    mouthCues = json.load(rhubarb_outfile)

                    character["animations"][f"say_{node_id}"] = {}
                    character["animations"][f"say_{node_id}"]["slots"] = {}

                    skins = [skin["name"] for skin in character["skins"]]
                    for skin in skins:
                        animation = [
                            {
                                "time": cues["start"],
                                "name": f"{skin}/mouth-{str(cues['value']).lower()}",
                            }
                            for cues in mouthCues["mouthCues"]
                        ]

                        character["animations"][f"say_{node_id}"]["slots"][
                            f"{skin}-mouth"
                        ] = {}
                        character["animations"][f"say_{node_id}"]["slots"][
                            f"{skin}-mouth"
                        ]["attachment"] = animation

            character_file.seek(0)
            character_file.write(json.dumps(character, indent=4))
            character_file.truncate()

        if set_read_only:
            character_path.chmod(S_IREAD | S_IRGRP | S_IROTH)


def get_notes(character=None) -> list[tuple[str, str, dict[str, str]]]:
    global db
    nodes = []
    for _root, _dirs, files in walk(SCHNACKER_FOLDER):
        for file in files:
            if file.endswith(".schnack"):
                dialog_file = Path(_root + file).read_text(encoding="utf-8")
                dialogs = json.loads(dialog_file)
                for dialog in dialogs["dialogs"]:
                    for node in dialog["nodes"]:
                        if "character" not in node or (
                            character and node["character"] != character
                        ):
                            continue

                        character_name = node["character"]

                        if "character" not in node or node["character"] in ["Player"]:
                            character_name = "joy"

                        if dialogs["localization"].get(str(node["id"])):
                            node_id = str(node["id"]).zfill(3)

                            for lang_code in dialogs["locales"]:
                                if not Path(
                                    f"data/audio/{lang_code}_{node_id}.ogg"
                                ).exists():
                                    print(
                                        colored(
                                            f"Can not load: data/audio/{lang_code}_{node_id}.ogg",
                                            "red",
                                        )
                                    )
                                    continue

                                nodes.append(
                                    (f"{lang_code}_{node_id}", character_name, db)
                                )
    return nodes


def spine_reexport(directorys: list[str]) -> None:
    global db
    spinefiles = []
    for folder in directorys:
        for root, _dirs, files in walk(folder):
            for file in files:
                if file.endswith(".spine"):
                    spinefiles.append((root + "/" + file, db))

    progress = Progress(0, len(spinefiles))
    progress.updateTitle(" Re-Exporting Spine Files")
    results: list[FileErrors] = []
    with Pool(SPINE_THREADS) as p:
        for i, (errors, spine_object, spine_file, db_out) in enumerate(
            p.imap_unordered(spine_export, spinefiles), 0
        ):
            progress.advance()

            spine_objects.update(spine_object)
            if not errors:
                parse_spine_json(spine_file=spine_file)
            if len(errors) > 0:
                results.append(
                    {
                        "file": spinefiles[i][0],
                        "errors": errors,
                    }
                )
            db = db | db_out
    progress.finish()

    for result in results:
        printErrors(result["file"], result["errors"])


def printErrors(fileName: str, errors: list[str]) -> None:
    color: Literal["red", "yellow"] = "red" if len(errors) > 0 else "yellow"
    print(colored(f"  {fileName}", color))
    for error in errors:
        print(colored(f"    ERROR: {error}", "red"))
    print()


def parse_spine_json(spine_file: str) -> None:
    # read spine json to check for missing scripts
    spine_json = Path(spine_file).read_text(encoding="utf-8")
    spine_object = json.loads(spine_json)

    if "animations" in spine_object:
        for animation in spine_object["animations"]:
            if animation in spine_animations:
                spine_animations[animation].add(spine_file)
            else:
                spine_animations[animation] = {spine_file}

    if "skins" in spine_object:
        for skin in spine_object["skins"]:
            if skin["name"] in spine_skins:
                spine_skins[skin["name"]].add(spine_file)
            else:
                spine_skins[skin["name"]] = {spine_file}

            if "attachments" not in skin:
                continue

            for attachment in skin["attachments"]:
                for subattachment in skin["attachments"][attachment]:
                    if (
                        "type" in skin["attachments"][attachment][subattachment]
                        and skin["attachments"][attachment][subattachment]["type"]
                        == "point"
                    ):
                        point_name = subattachment
                        if point_name in spine_points:
                            spine_points[point_name].add(spine_file)
                        else:
                            spine_points[point_name] = {spine_file}

        for i, _skin in enumerate(spine_object["skins"]):
            if "attachments" not in spine_object["skins"][i]:
                continue

            attachment_obj = spine_object["skins"][i]["attachments"]
            for attachment in attachment_obj:
                for subattachment in attachment_obj[attachment]:
                    if "name" in attachment_obj[attachment][subattachment]:
                        bbname = attachment_obj[attachment][subattachment]["name"]
                    else:
                        bbname = subattachment

                    if (
                        "type" in attachment_obj[attachment][subattachment]
                        and attachment_obj[attachment][subattachment]["type"]
                        == "boundingbox"
                    ):
                        if (
                            bbname == "walkable_area" or bbname == "non_walkable_area"
                        ):  # No scripts for navmeshes
                            continue
                        if (
                            not bbname.startswith("dlg:")
                            and not bbname.startswith("anim:")
                            and not Path(f"./data-src/scripts/{bbname}.lua").exists()
                        ):
                            if not Path("./data-src/scripts/").exists():
                                Path("./data-src/scripts/").mkdir(
                                    parents=True, exist_ok=True
                                )
                            with Path(f"./data-src/scripts/{bbname}.lua").open(
                                "w"
                            ) as f:
                                f.write(f'print("{bbname}")')
                            print(
                                colored(
                                    f"Script {bbname}.lua was created automatically!",
                                    "blue",
                                )
                            )
                        if bbname.startswith("dlg:") and bbname[4:] not in all_dialogs:
                            print(
                                colored(
                                    f"Dialog {bbname[4:]} is missing! Found in {spine_file}.",
                                    "red",
                                )
                            )


# there is no print allowed in this function, since this would destroy the progress bar
def spine_export(
    in_tuple,
) -> (
    tuple[list[str], dict[Any, Any], Literal[""], dict[str, str]]
    | tuple[list[Any], dict[str, set[str]], str, dict[str, str]]
):
    file: str = in_tuple[0]
    db: dict[str, str] = in_tuple[1]
    errors = []
    if not file.endswith(".spine"):
        errors.append("invalid spine file " + file)
        return errors, {}, "", {}

    if not Path(SPINE).exists():
        errors.append("spine executable '" + SPINE + "' could not be found!")
        return errors, {}, "", {}

    name = Path(file).stem

    db_out: dict[str, str] = {}
    with open(file, "rb") as f:
        checksum = hashlib.file_digest(f, "sha256").hexdigest()
        db_out[file] = checksum
        if db.get(file, None) == checksum:
            return errors, {name: {file}}, f"./data/{name}/{name}.json", db_out

    command: list[str] = [
        SPINE,
        "-i",
        file,
        "-m",
        "-o",
        f"./data/{name}/",
        "-e",
        "./data-src/spine_export_template.export.json",
    ]
    with contextlib.suppress(Exception):
        shutil.rmtree(f"./data/{name}/", ignore_errors=True)
        if set_read_only:
            Path(f"./data/{name}/{name}.json").chmod(
                S_IREAD | S_IRGRP | S_IROTH | S_IWUSR
            )
            Path(f"./data/{name}/{name}.atlas").chmod(
                S_IREAD | S_IRGRP | S_IROTH | S_IWUSR
            )
    p = subprocess.Popen(
        command, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    output = p.communicate()[0]
    if p.returncode != 0:
        print(colored(output.decode(), "red"))
        sys.exit(p.returncode)

    if not Path(f"./data/{name}/{name}.json").exists():
        errors.append(
            f"Spine export of {name} failed. No file ./data/{name}/{name}.json was created. \n"
            f"Is the sceleton of {name}.spine named {name}?"
        )
        return errors, {}, "", {}

    if (Path(file).parent / Path("zBufferMap")).exists():
        copy_folder(str(Path(file).parent / Path("zBufferMap")), f"./data/{name}/")

    if set_read_only:
        with contextlib.suppress(Exception):
            Path(f"./data/{name}/{name}.json").chmod(S_IREAD | S_IRGRP | S_IROTH)
            Path(f"./data/{name}/{name}.atlas").chmod(S_IREAD | S_IRGRP | S_IROTH)

    return errors, {name: {file}}, f"./data/{name}/{name}.json", db_out


def scripts_recopy(directorys: list[str]) -> None:
    for folder in directorys:
        for root, _dirs, files in walk(folder):
            for file in files:
                if file.endswith(".lua"):
                    copy_script(file=root + "/" + file)


def rehash_scenes(directorys: str) -> None:
    for root, _dirs, files in walk(directorys):
        for file in files:
            if file.endswith(".json"):
                if file in all_scenes:
                    all_scenes[file.removesuffix(".json")].add("data-src")
                else:
                    all_scenes[file.removesuffix(".json")] = {"data-src"}

                src_path = root + "/" + file
                try:
                    data = Path(src_path).read_text(encoding="utf-8")
                    parsed = json.loads(data)

                    for item in parsed["items"]:
                        if "id" in item:
                            spine_objects.update({item["id"]: file})

                    old_hash = parsed["hash"]
                    parsed["hash"] = ""
                    new_hash = hashlib.sha1(str(parsed).encode()).hexdigest()
                    if old_hash == new_hash:
                        continue
                    parsed["hash"] = new_hash
                    with Path(src_path).open("w") as f:
                        scene_files.append(src_path)
                        f.write(json.dumps(parsed, indent=4))
                except Exception:
                    print(colored(f"data-src file '{src_path}' has errors", "red"))


def copy_script(file: str) -> None:
    if file.endswith("ALPACA.lua"):
        return
    if file.endswith(".lua"):
        command = [LUA, "-p", file]
        p = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        output = p.communicate()[0]
        if p.returncode != 0:
            print(colored(output.decode(), "red"))

        name = Path(file).name
        Path("./data/scripts").mkdir(parents=True, exist_ok=True)

        if set_read_only:
            with contextlib.suppress(Exception):
                Path(f"./data/scripts/{name}").chmod(
                    S_IREAD | S_IRGRP | S_IROTH | S_IWUSR
                )
        shutil.copyfile(file, f"./data/scripts/{name}")
        if set_read_only:
            Path(f"./data/scripts/{name}").chmod(S_IREAD | S_IRGRP | S_IROTH)


def copy_folder(src: str, des: str) -> None:
    for root, _dirs, files in walk(src):
        for file in files:
            copy_file(src=root + "/" + file, des=Path(des))
            if ".schnack" in file:
                fill_all_dialogs(Path(root + "/" + file), file)
            if ".ogg" in file:
                all_audio[file] = {src}


def copy_file(src: str, des: Path) -> None:
    src = src.replace("\\", "/")
    name = Path(src).name
    Path(des).mkdir(parents=True, exist_ok=True)

    if set_read_only:
        with contextlib.suppress(Exception):
            Path(f"{des}/{name}").chmod(S_IREAD | S_IRGRP | S_IROTH | S_IWUSR)
    with contextlib.suppress(FileNotFoundError):
        shutil.copyfile(src, f"{des}/{name}")
        if set_read_only:
            Path(f"{des}/{name}").chmod(S_IREAD | S_IRGRP | S_IROTH)


def fill_all_dialogs(path: Path, file: str) -> None:
    data = Path(path).read_text(encoding="utf-8")
    dialogs = json.loads(data)
    for dialog in dialogs["dialogs"]:
        if dialog["name"] in all_dialogs:
            all_dialogs[dialog["name"]].add(file)
        else:
            all_dialogs[dialog["name"]] = {file}
    for character in dialogs["characters"]:
        schnack_characters_props[f"{character['canonicalName']}"] = "{}"
        for prop, value in character["properties"].items():
            schnack_characters_props[f"{character['canonicalName']}.{prop}"] = value

    for variable, value in dialogs["variables"].items():
        schnack_vars[variable] = value
    for language in dialogs["locales"]:
        all_language[language] = {file}


def on_moved(event) -> None:
    print(
        colored(
            f"ok ok ok, someone moved {event.src_path} to {event.dest_path}", "green"
        )
    )


def on_created(event) -> None:
    print(colored(f"{event.src_path} has been created!", "green"))


def on_deleted(event) -> None:
    print(
        colored(
            f"{event.src_path} was deleted! Please delete it manually from data.", "red"
        )
    )


def on_data_src_modified(event) -> None:
    global db
    time.sleep(0.5)
    print(colored(f"data-src file '{event.src_path}' has been modified", "green"))
    if event.src_path.endswith(".spine"):
        errors, spine_object, spine_file, db = spine_export((event.src_path, db))
        spine_objects.update(spine_object)
        if not errors:
            parse_spine_json(spine_file=spine_file)
        if len(errors) > 0:
            printErrors(event.src_path, errors)
        if spine_object.keys():
            apply_rhubarb(list(spine_object.keys())[0])
    if event.src_path.endswith(".ogg"):
        errors, db = rhubarb_export(node_info=(event.src_path, "", db))
        if len(errors) > 0:
            printErrors(event.src_path, errors)
    if event.src_path.endswith(".lua"):
        copy_script(event.src_path)
    if event.src_path.endswith(".json") and "scenes" in event.src_path:
        file = Path(event.src_path).name
        if event.src_path in scene_files:
            scene_files.remove(event.src_path)
            copy_file(f"./data-src/scenes/{file}", Path("./data/scenes"))
            scene = file.removesuffix(".json")
            if not Path(f"./data-src/scripts/{scene}.lua").exists():
                if not Path("./data-src/scripts/").exists():
                    Path("./data-src/scripts/").mkdir(parents=True, exist_ok=True)
                with Path(f"./data-src/scripts/{scene}.lua").open("w") as f:
                    f.write(f'print("{scene}")')
                    print(
                        colored(f"Script {scene}.lua was created automatically!"),
                        "blue",
                    )
            if file.removesuffix(".json") in all_scenes:
                all_scenes[file.removesuffix(".json")].add("data-src")
            else:
                all_scenes[file.removesuffix(".json")] = {"data-src"}
            return
        parsed = None
        try:
            data = Path(event.src_path).read_text(encoding="utf-8")
            parsed = json.loads(data)
            old_hash = parsed["hash"]
            parsed["hash"] = ""
            new_hash = hashlib.sha1(str(parsed).encode()).hexdigest()
            if old_hash == new_hash:
                return
            parsed["hash"] = new_hash
            with Path(event.src_path).open("w") as f:
                scene_files.append(event.src_path)
                f.write(json.dumps(parsed, indent=4))
        except Exception:
            print(colored(f"data-src file '{event.src_path}' has errors", "red"))
    if event.src_path.endswith(".json") and "config" in event.src_path:
        file = Path(event.src_path).name
        copy_file(f"./data-src/config/{file}", Path("./data/config"))
    if event.src_path.endswith(".schnack") and "dialog" in event.src_path:
        file = Path(event.src_path).name
        copy_file(f"./data-src/dialog/{file}", Path("./data/dialog"))
        fill_all_dialogs(Path(f"./data-src/dialog/{file}"), file)
    if "ALPACA.lua" not in event.src_path:
        LuaDocsGen().render("lua.cpp")


@dataclass
class DocFunction:
    """Class for keeping track of an item in inventory."""

    name: str = ""
    docs: list[str] = field(default_factory=list)
    parameters: str = ""
    copy_parameters: str = ""
    returns: str = ""


class LuaDocsGen:
    """The PAC's Lua handler class."""

    def collect(self: LuaDocsGen, identifier: str) -> str:
        return identifier

    def get_name(self: LuaDocsGen, name: str) -> str:
        result = name.replace('lua_state->set_function("', "")
        result = result.replace('"', "")
        result = result.replace(",", "")
        return result.strip()

    def get_parameters(self: LuaDocsGen, parameters: str) -> str:
        result = parameters.replace("[](", "").replace(")", "")
        result = result.replace("[this](", "").replace(")", "")
        result = result.replace("const ", "")
        result = result.replace("std::string ", "string ")
        result = result.replace("&", "")
        result = result.replace("\tint ", "number ")
        result = result.replace(", int ", ", number ")
        result = result.replace("float ", "number ")
        result = result.replace("bool ", "boolean ")
        result = result.replace("sol::function ", "function ")
        result = result.replace("std::optional<sol::function> ", "function? ")
        result = result.replace("std::vector<LuaSpineSkin> ", "LuaSpineSkin[] ")

        return result.strip()

    def get_copy_parameters(self: LuaDocsGen, parameters: str) -> str:
        result = self.get_parameters(parameters)
        result = result.replace("string ", "")
        result = result.replace("number ", "")
        result = result.replace("function ", "")
        result = result.replace("function? ", "")
        result = result.replace("boolean ", "")
        result = result.replace("LuaSpineObject ", "")
        result = result.replace("LuaSpineAnimation ", "")
        result = result.replace("LuaSpineSkin ", "")
        result = result.replace("LuaSpinePoint ", "")
        result = result.replace("LuaDialog ", "")
        result = result.replace("LuaScene ", "")
        result = result.replace("LuaAudio ", "")
        result = result.replace("LuaLanguage ", "")
        result = result.replace("LuaSpineSkin[] ", "")

        return result.strip()

    def get_docs(self: LuaDocsGen, code: list[str], index: int) -> list[str]:
        result = []
        index -= 1

        while "///" in code[index]:
            if "returns:" in code[index]:
                index -= 1
                continue

            result.append(code[index].replace("///", "").strip())
            index -= 1

        if result == []:
            return ["Cool Documentation"]

        result.reverse()
        return result

    def get_returns(self: LuaDocsGen, code: list[str], index: int) -> str:
        result = ""
        index -= 1

        while "///" in code[index]:
            if "return" in code[index]:
                result += code[index].replace("///", "").strip()
            index -= 1

        if result == "":
            return "void"

        return result.strip()

    def render(self: LuaDocsGen, data: str) -> str:
        template = ""
        result = []
        codes = []

        code_file = Path("subprojects/ALPACA/src/" + data).absolute()
        if code_file.exists():
            with code_file.open() as f:
                codes = f.readlines()

        if not codes:
            code: str = requests.get(
                "https://raw.githubusercontent.com/pinguin999/ALPACA/main/src/lua.cpp",
                timeout=30,
            ).text
            codes = code.split("\n")

        for i, line in enumerate(codes):
            if "set_function" in line:
                doc_obj = DocFunction()
                doc_obj.name = self.get_name(line)
                doc_obj.docs = self.get_docs(codes, i)
                doc_obj.parameters = self.get_parameters(codes[i + 1])
                doc_obj.copy_parameters = self.get_copy_parameters(codes[i + 1])
                doc_obj.returns = self.get_returns(codes, i)
                result.append(doc_obj)

        alpaca_lua = Path("data-src/scripts/ALPACA.lua")
        if not alpaca_lua.exists():
            alpaca_lua.touch(exist_ok=True)
        with alpaca_lua.open("w") as output:
            output.write("")  # Clear file

        def write_alias(alias: str, entries: dict[str, set[str]]) -> None:
            with alpaca_lua.open("+a") as output:
                output.write(f"\n\n---@alias {alias}")
                for spine_object in entries.items():
                    output.write(
                        f"""\n---| '"{spine_object[0]}"' # Found in {spine_object[1]}"""
                    )
                output.write("\n")

        write_alias("LuaSpineObject", spine_objects)
        write_alias("LuaSpineAnimation", spine_animations)
        write_alias("LuaSpineSkin", spine_skins)
        write_alias("LuaSpinePoint", spine_points)
        write_alias("LuaDialog", all_dialogs)
        write_alias("LuaScene", all_scenes)
        write_alias("LuaAudio", all_audio)
        write_alias("LuaLanguage", all_language)

        with alpaca_lua.open("+a") as output:
            for func in result:
                docs = f"--- {func.docs[0]}"
                for doc in func.docs[1:]:
                    docs += f"\n-- {doc}"

                parameters = func.parameters.split(",")
                for parameter in parameters:
                    parameter_striped = parameter.strip()
                    if parameter_striped == "":
                        continue
                    parameter_striped = parameter_striped.split(" ")
                    if parameter_striped[0] == "function?":
                        parameter_striped[0] = "function"
                        parameter_striped[1] = parameter_striped[1] + "?"
                    docs += f"\n---@param {parameter_striped[1]} {parameter_striped[0]}"
                output.write(f"""
{docs}
function {func.name}({func.copy_parameters})
end
""")

            output.write("\ninventory_items = {}\n")
            for schnack_var, value in schnack_vars.items():
                if value in ["{}", False, True]:
                    output.write(f"""
{schnack_var} = {str(value).lower()}""")
                else:
                    output.write(f"""
{schnack_var} = "{value}" """)

            output.write("\ncharacters = {}")
            for schnack_characters_prop, value in schnack_characters_props.items():
                if value in ["{}", False, True]:
                    output.write(f"""
characters.{schnack_characters_prop} = {str(value).lower()}""")
                else:
                    output.write(f"""
characters.{schnack_characters_prop} = "{value}" """)

            output.write("\nscenes = {}")
            for scene in all_scenes.keys():
                output.write(
                    f"""
scenes.{scene} = {{}}
scenes.{scene}.items = {{}}
scenes.{scene}.bottom_border = 0
scenes.{scene}.hash = 0
scenes.{scene}.left_border = 0
scenes.{scene}.right_border = 0
scenes.{scene}.top_border = 0
scenes.{scene}.zBufferMap = nil """
                    ""
                )
                scene_json = Path(f"./data-src/scenes/{scene}.json").read_text(
                    encoding="utf-8"
                )
                try:
                    scene_object = json.loads(scene_json)
                    for item in scene_object["items"] + [{"spine": "background"}]:
                        name = item["id"] if "id" in item else item["spine"]
                        output.write(f"""
scenes.{scene}.items.{name} = {{}}
scenes.{scene}.items.{name}.x = 0
scenes.{scene}.items.{name}.loop_animation = false
scenes.{scene}.items.{name}.layer = 0
scenes.{scene}.items.{name}.spine = "{name}"
scenes.{scene}.items.{name}.scale = 1
scenes.{scene}.items.{name}.animation = "idle"
scenes.{scene}.items.{name}.visible = true
scenes.{scene}.items.{name}.abs_position = false
scenes.{scene}.items.{name}.y = 0
scenes.{scene}.items.{name}.cross_scene = false
scenes.{scene}.items.{name}.skin = 0""")

                except Exception:
                    pass
            output.write("\nconfig = {}")
            output.write("""
game = {}
game["old_scene"] = ""
game["scene"] = ""
game["interruptible"] = true""")

        return template


if __name__ == "__main__":
    freeze_support()
    global db
    try:
        with open("db", "rb") as handle:
            db = pickle.load(handle)
    except FileNotFoundError:
        db = {"a": "b"}
    print(colored("Start convert", "green"))
    scripts_recopy(["./data-src/scripts/"])
    copy_folder("./data-src/config", "./data/config")
    copy_folder("./data-src/fonts", "./data/fonts")
    copy_folder("./data-src/scenes", "./data/scenes")
    copy_folder("./data-src/audio", "./data/audio")
    copy_folder("./data-src/icons", "./data/icons")
    copy_folder("./data-src/dialog", "./data/dialog")
    spine_reexport(["./data-src"])
    rehash_scenes("./data-src/scenes")
    rhubarb_path = Path(RHUBARB)
    if rhubarb_path.exists():
        rhubarb_reexport()
        apply_rhubarb()
    else:
        print(colored("Rhubarb was skipped, since Rhubarb path was not found", "red"))
    LuaDocsGen().render("lua.cpp")
    print(colored("Convert sucess", "green"))
    patterns_src = ["*.spine", "*.lua", "*.json", "*.schnack", "*.ogg"]
    ignore_patterns = None
    ignore_directories = False
    case_sensitive = True

    go_recursively = True
    data_src_event_handler = PatternMatchingEventHandler(
        patterns=patterns_src,
        ignore_patterns=ignore_patterns,
        ignore_directories=ignore_directories,
        case_sensitive=case_sensitive,
    )
    data_src_event_handler.on_created = on_created
    data_src_event_handler.on_deleted = on_deleted
    data_src_event_handler.on_modified = on_data_src_modified
    data_src_event_handler.on_moved = on_moved

    data_src_path = "./data-src/"
    data_src_observer = Observer()
    data_src_observer.schedule(
        data_src_event_handler, data_src_path, recursive=go_recursively
    )
    data_src_observer.start()

    with open("db", "wb") as whandle:
        pickle.dump(db, whandle, protocol=pickle.HIGHEST_PROTOCOL)
    print(colored("Observer Started", "green"))

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        data_src_observer.stop()
        data_src_observer.join()
