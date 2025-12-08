#pragma once

#include <string>
#include <map>
#include <jngl.hpp>

class AudioManager
{
public:
    AudioManager();
    void loopMusic(const std::string &filePath);
    void stopMusic();

    void setSoundVolume(float volume);
    void setVoiceVolume(float volume);
    void setMusicVolume(float volume);

    float getSoundVolume() const;
    float getVoiceVolume() const;
    float getMusicVolume() const;

private:
    std::string currentMusic;
    std::map<std::string, std::shared_ptr<jngl::SoundFile>> loadedSounds = {};

    float soundVolume = 0.8f;
    float voiceVolume = 1.0f;
    float musicVolume = 0.5f;
};
