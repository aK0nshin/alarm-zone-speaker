#pragma once

class TrackPlayer {
public:
    virtual ~TrackPlayer() = default;
    virtual bool play(const char* path) = 0;
    virtual void stop() = 0;
    virtual void tick() = 0;
    virtual bool isPlaying() const = 0;
    virtual bool hasError() const = 0;
};
