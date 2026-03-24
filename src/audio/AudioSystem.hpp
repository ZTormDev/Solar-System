#pragma once

#include <string>

struct MIX_Mixer;
struct MIX_Audio;
struct MIX_Track;

class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void init();
    void shutdown();

    void playMusic(const std::string& filePath, float volume = 1.0f, bool loop = true);
    void stopMusic();
    void setMusicVolume(float volume);

    bool isInitialized() const { return initialized; }

private:
    bool initialized = false;
    MIX_Mixer* mixer = nullptr;
    MIX_Audio* musicAudio = nullptr;
    MIX_Track* musicTrack = nullptr;
};
