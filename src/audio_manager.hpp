#pragma once

#include <string>
#include <map>
#include <jngl.hpp>

struct Channels : public jngl::Singleton<Channels> {
    jngl::Channel& main = jngl::Channel::main();
    jngl::Channel music;
    jngl::Channel voice;
    jngl::Channel sounds;
};

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

    float soundVolume = 1.0f;
    float voiceVolume = 1.0f;
    float musicVolume = 1.0f;
};
