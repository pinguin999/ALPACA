#include "audio_manager.hpp"


void AudioManager::step(){
    if (fadeStep){
        fadeStep.value() -= 0.05f;
        if (fadeStep.value() > 0)
        {
            Channels::handle().music_fadeout.setVolume(musicVolume + fadeStep.value() * (0.0f - musicVolume));
            Channels::handle().music.setVolume(0.0f + fadeStep.value() * (musicVolume - 0.0f));
            // jngl::debug("Down A Volume fade {} Volume Music {}", musicVolume + fadeStep.value() * (0.0f - musicVolume), 0.0f + fadeStep.value() * (musicVolume - 0.0f));
        } else {
            Channels::handle().music_fadeout.setVolume(0.0f + fadeStep.value() * (musicVolume - 0.0f));
            Channels::handle().music.setVolume(musicVolume + fadeStep.value() * (0.0f - musicVolume));
            // jngl::debug("Up A Volume fade {} Volume Music {}", musicVolume + fadeStep.value() * (0.0f - musicVolume), 0.0f + fadeStep.value() * (musicVolume - 0.0f));
        }
    }
}

void AudioManager::fadeLoopMuisc(const std::string &filePath){
    // if the music is the same as already playing, do nothing
    if (filePath == currentMusic)
    {
        fadeStep = std::nullopt;
        return;
    }

    Channels::handle().music_fadeout = std::move(Channels::handle().music);
    Channels::handle().music = jngl::Channel();
    Channels::handle().music_fadeout.setVolume(musicVolume);
    fadeStep = 1.0f;

    // now start the new music
    currentMusic = filePath;
    Channels::handle().music.loop(filePath);
    Channels::handle().music.setVolume(0);
}

void AudioManager::stopMusic()
{
    if(currentMusic.empty())
    {
        return;
    }

    Channels::handle().music.stop(currentMusic);
    currentMusic = "";
}


void AudioManager::stopFadeMusic()
{
    Channels::handle().music_fadeout.stopAll();
}

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

void AudioManager::loopAmbient(const std::string &filePath)
{
    // if the music is the same as already playing, do nothing
    if (currentAmbient.contains(filePath))
    {
        return;
    }

    // now start the ambient sounds
    currentAmbient.insert(filePath);
    Channels::handle().ambient.loop(filePath);
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

void AudioManager::setAmbientVolume(float volume)
{
    ambientVolume = volume;
    Channels::handle().ambient.setVolume(ambientVolume);
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

float AudioManager::getAmbientVolume() const
{
    return ambientVolume;
}
