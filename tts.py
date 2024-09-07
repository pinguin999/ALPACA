"""
    TTS idea for ALPACA
"""
from pathlib import Path
import subprocess
from google.cloud import texttospeech
import json

SRC_PATH = "data-src/dialog/2080.schnack"

# Instantiates a client
client = texttospeech.TextToSpeechClient()

# Build the voice request, select the language code ("en-US") and the ssml
# voice gender ("neutral")
voice = texttospeech.VoiceSelectionParams(
    language_code="de-DE",
    ssml_gender=texttospeech.SsmlVoiceGender.FEMALE,
    name="de-DE-Neural2-A",
)

# Select the type of audio file you want returned
audio_config = texttospeech.AudioConfig(
    audio_encoding=texttospeech.AudioEncoding.MP3,
    speaking_rate=1.23,
    effects_profile_id=["handset-class-device"],
)

with Path(SRC_PATH).open() as f:
    data = f.read()
    parsed = json.loads(data)
    localizations = parsed["localization"]

    for localization in localizations:
        out_path_temp: Path = Path(f"./data-src/audio/voice/de_{localization}.ogg_temp")
        out_path: Path = Path(f"./data-src/audio/voice/de_{localization}.ogg")

        if not out_path_temp.exists():
            print(f"{localization}.ogg")
            # Set the text input to be synthesized
            synthesis_input = texttospeech.SynthesisInput(
                text=localizations[localization][0]
            )
            # Perform the text-to-speech request on the text input with the selected
            # voice parameters and audio file type
            response = client.synthesize_speech(
                input=synthesis_input, voice=voice, audio_config=audio_config
            )

            # The response's audio_content is binary.
            with out_path_temp.open("wb") as out:
                # Write the response to the output file.
                out.write(response.audio_content)
                print(f'Audio content written to file "{out_path_temp}"')

        command = [
            "ffmpeg",
            "-i",
            str(out_path_temp),
            "-c:a",
            "vorbis",
            "-ac",
            "2",
            "-strict",
            "experimental",
            str(out_path),
            "-y",
        ]
        p = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        output = p.communicate()[0]
        out_path_temp.unlink(missing_ok=True)
