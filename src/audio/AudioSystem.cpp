#include "AudioSystem.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>

AudioSystem::~AudioSystem() {
    if (initialized) {
        shutdown();
    }
}

void AudioSystem::init() {
    if (initialized) {
        return;
    }

    if (!MIX_Init()) {
        throw std::runtime_error(std::string("Failed to initialize SDL_mixer library: ") + SDL_GetError());
    }

    // open default playback device with default spec
    mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!mixer) {
        MIX_Quit();
        throw std::runtime_error(std::string("Failed to create SDL_mixer device: ") + SDL_GetError());
    }

    // pre-create a music track
    musicTrack = MIX_CreateTrack(mixer);
    if (!musicTrack) {
        MIX_DestroyMixer(mixer);
        mixer = nullptr;
        MIX_Quit();
        throw std::runtime_error(std::string("Failed to create SDL_mixer track: ") + SDL_GetError());
    }

    std::cout << "[AudioSystem] Initialized (SDL3_mixer 3.x)." << std::endl;
    initialized = true;
}

void AudioSystem::shutdown() {
    if (!initialized) {
        return;
    }

    stopMusic();

    if (musicTrack) {
        MIX_DestroyTrack(musicTrack);
        musicTrack = nullptr;
    }

    if (mixer) {
        MIX_DestroyMixer(mixer);
        mixer = nullptr;
    }

    MIX_Quit();
    initialized = false;

    std::cout << "[AudioSystem] Shut down." << std::endl;
}

void AudioSystem::playMusic(const std::string& filePath, float volume, bool loop) {
    if (!initialized) {
        std::cerr << "[AudioSystem] Cannot play music: not initialized." << std::endl;
        return;
    }

    stopMusic();

    namespace fs = std::filesystem;
    fs::path resolvedPath = fs::absolute(filePath);

    if (!fs::exists(resolvedPath)) {
        std::cerr << "[AudioSystem] File not found: " << resolvedPath.string() << std::endl;
        return;
    }

    std::string pathStr = resolvedPath.string();
    musicAudio = MIX_LoadAudio(mixer, pathStr.c_str(), false);
    if (!musicAudio) {
        std::cerr << "[AudioSystem] Failed to load: " << pathStr
                  << " (" << SDL_GetError() << ")" << std::endl;
        return;
    }

    if (!MIX_SetTrackAudio(musicTrack, musicAudio)) {
        std::cerr << "[AudioSystem] Failed to set track audio: " << SDL_GetError() << std::endl;
        MIX_DestroyAudio(musicAudio);
        musicAudio = nullptr;
        return;
    }

    MIX_SetTrackGain(musicTrack, volume);
    MIX_SetTrackLoops(musicTrack, loop ? -1 : 0);

    if (!MIX_PlayTrack(musicTrack, 0)) {
        std::cerr << "[AudioSystem] Failed to play track: " << SDL_GetError() << std::endl;
        MIX_SetTrackAudio(musicTrack, nullptr);
        MIX_DestroyAudio(musicAudio);
        musicAudio = nullptr;
        return;
    }

    std::cout << "[AudioSystem] Playing: " << pathStr
              << " (volume=" << volume << ", loop=" << (loop ? "true" : "false") << ")" << std::endl;
}

void AudioSystem::stopMusic() {
    if (musicTrack) {
        MIX_StopTrack(musicTrack, 0);
        MIX_SetTrackAudio(musicTrack, nullptr);
    }
    if (musicAudio) {
        MIX_DestroyAudio(musicAudio);
        musicAudio = nullptr;
    }
}

void AudioSystem::setMusicVolume(float volume) {
    if (!initialized || !musicTrack) {
        return;
    }
    MIX_SetTrackGain(musicTrack, volume);
}
