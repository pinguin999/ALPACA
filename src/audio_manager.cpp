#include "audio_manager.hpp"
#include "game.hpp"

AudioManager::AudioManager()
{
}

void AudioManager::stopMusic()
{
    if(currentMusic != nullptr)
        currentMusic->stop();

    currentMusic = nullptr;
}

// TODO: implement crossfade using an additional volume factor
void AudioManager::loopMusic(const std::string filePath)
{
    auto sound = loadSound(filePath);
    // if the music is the same as already playing, do nothing
    if(sound == currentMusic)
        return;

    // if we need to change the music, stop the old one first
    if(currentMusic != nullptr)
        currentMusic->stop();

    // now start the new music
    currentMusic = sound;
    currentMusic->loop();
    currentMusic->setVolume(musicVolume);
}

void AudioManager::setSoundVolume(float volume)
{
    soundVolume = volume;
}

void AudioManager::setVoiceVolume(float volume)
{
    voiceVolume = volume;
}

void AudioManager::setMusicVolume(float volume)
{
    musicVolume = volume;
    currentMusic->setVolume(musicVolume);
}

float AudioManager::getSoundVolume()
{
    return soundVolume;
}

float AudioManager::getVoiceVolume()
{
    return voiceVolume;
}

float AudioManager::getMusicVolume()
{
    return musicVolume;
}

std::shared_ptr<jngl::SoundFile> AudioManager::loadSound(std::string filePath)
{
    if(loadedSounds.count(filePath))
    {
        return loadedSounds[filePath];
    }
    auto sound = std::make_shared<jngl::SoundFile>(filePath);
    loadedSounds[filePath] = sound;
    return sound;
}
