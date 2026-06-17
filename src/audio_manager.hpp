#pragma once

#include <map>
#include <jngl.hpp>

struct Channels : public jngl::Singleton<Channels> {
    jngl::Channel& main = jngl::Channel::main();
    jngl::Channel music;
    jngl::Channel voice;
    jngl::Channel sounds;
    jngl::Channel ambient;

    jngl::Channel music_fadeout;
};

class AudioManager : public jngl::Singleton<AudioManager>
{
public:
    void step();
    void fadeLoopMuisc(const std::string &filePath);

    void loopMusic(const std::string &filePath);
    void stopMusic();

    void stopFadeMusic();

    void setSoundVolume(float volume);
    void setVoiceVolume(float volume);
    void setMusicVolume(float volume);
    void setAmbientVolume(float volume);

    float getSoundVolume() const;
    float getVoiceVolume() const;
    float getMusicVolume() const;
    float getAmbientVolume() const;

private:
    std::string currentMusic;
    std::map<std::string, std::shared_ptr<jngl::SoundFile>> loadedSounds = {};

    float soundVolume = 1.0f;
    float voiceVolume = 1.0f;
    float musicVolume = 1.0f;
    float ambientVolume = 1.0f;

    std::optional<float> fadeStep;
};
