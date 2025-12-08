#include "audio_manager.hpp"


struct Channels : public jngl::Singleton<Channels>{
    jngl::Channel& main = jngl::Channel::main();
    jngl::Channel music;
    jngl::Channel voice;
    jngl::Channel sounds;
};

AudioManager::AudioManager() = default;

void AudioManager::stopMusic()
{
    Channels::handle().music.stop(currentMusic);
    currentMusic = "";
}

// TODO: implement crossfade using an additional volume factor
void AudioManager::loopMusic(const std::string &filePath)
{
    // if the music is the same as already playing, do nothing
    if (filePath == currentMusic)
    {
        return;
    }

    // if we need to change the music, stop the old one first
    if (!currentMusic.empty())
    {
        Channels::handle().music.stop(currentMusic);
    }

    // now start the new music
    currentMusic = filePath;
    Channels::handle().music.loop(filePath);
}

void AudioManager::setSoundVolume(float volume)
{
    soundVolume = volume;
    Channels::handle().sounds.setVolume(soundVolume);
}

void AudioManager::setVoiceVolume(float volume)
{
    voiceVolume = volume;
    Channels::handle().voice.setVolume(voiceVolume);
}

void AudioManager::setMusicVolume(float volume)
{
    musicVolume = volume;
    Channels::handle().music.setVolume(musicVolume);
}

float AudioManager::getSoundVolume() const
{
    return soundVolume;
}

float AudioManager::getVoiceVolume() const
{
    return voiceVolume;
}

float AudioManager::getMusicVolume() const
{
    return musicVolume;
}


