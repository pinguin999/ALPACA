#pragma once

#include <string>
#include <map>
#include <jngl.hpp>

class AudioManager
{
public:
    AudioManager();
    void loopMusic(std::string filePath);
    void stopMusic();
    std::shared_ptr<jngl::SoundFile> loadSound(std::string filePath);

    void setSoundVolume(float volume);
    void setVoiceVolume(float volume);
    void setMusicVolume(float volume);

    float getSoundVolume();
    float getVoiceVolume();
    float getMusicVolume();

private:
    std::shared_ptr<jngl::SoundFile> currentMusic = {};
    std::map<std::string, std::shared_ptr<jngl::SoundFile>> loadedSounds = {};

    float soundVolume = 0.8f;
    float voiceVolume = 1.0f;
    float musicVolume = 0.5f;
};
