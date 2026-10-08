#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <cstdint>

// JUCE supplies positioning, read-ahead and sample-rate conversion. Playback
// is gated here so Stop never waits for a device callback to acknowledge it.
class TimelineAudioTransportSource : public juce::PositionableAudioSource,
                                     public juce::ChangeBroadcaster
{
public:
    void setSource(juce::PositionableAudioSource* source, int readAheadSize = 0,
        juce::TimeSliceThread* readAheadThread = nullptr, double sourceSampleRate = 0.0,
        int maxNumChannels = 2)
    {
        stop();
        const juce::ScopedLock lock(transportLock);
        transport.setSource(source, readAheadSize, readAheadThread, sourceSampleRate, maxNumChannels);
        sourceAttached = source != nullptr;
        stop();
    }

    void start()
    {
        const juce::ScopedLock lock(transportLock);
        auto state = playbackState.load();
        transport.start();
        const auto next = (state + 2) | (transport.isPlaying() ? 1u : 0u);
        // A Stop issued while start() acquired the transport cancels this start.
        if (playbackState.compare_exchange_strong(state, next) && !(state & 1u) && (next & 1u))
            sendChangeMessage();
    }

    void stop()
    {
        auto state = playbackState.load();
        while (!playbackState.compare_exchange_weak(state, (state + 2) & ~std::uint64_t{ 1 })) {}
        if (state & 1u) sendChangeMessage();
    }

    bool isPlaying() const noexcept { return (playbackState.load() & 1u) != 0; }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        if (!isPlaying()) { info.clearActiveBufferRegion(); return; }
        const juce::ScopedLock lock(transportLock);
        auto state = playbackState.load();
        if (!(state & 1u)) { info.clearActiveBufferRegion(); return; }
        transport.getNextAudioBlock(info);
        const bool finished = !transport.isPlaying()
            && playbackState.compare_exchange_strong(state, (state + 2) & ~std::uint64_t{ 1 });
        if (finished) sendChangeMessage();
        else if (!isPlaying()) info.clearActiveBufferRegion();
    }

    void prepareToPlay(int blockSize, double sampleRate) override
    {
        const juce::ScopedLock lock(transportLock);
        transport.prepareToPlay(blockSize, sampleRate);
    }

    void releaseResources() override
    {
        stop();
        const juce::ScopedLock lock(transportLock);
        transport.releaseResources();
    }

    void setPosition(double seconds)
    {
        const juce::ScopedLock lock(transportLock);
        transport.setPosition(seconds);
    }

    double getCurrentPosition() const
    {
        const juce::ScopedLock lock(transportLock);
        return transport.getCurrentPosition();
    }

    double getLengthInSeconds() const
    {
        const juce::ScopedLock lock(transportLock);
        return transport.getLengthInSeconds();
    }

    bool hasStreamFinished() const noexcept
    {
        const juce::ScopedLock lock(transportLock);
        return sourceAttached && transport.hasStreamFinished();
    }

    void setNextReadPosition(juce::int64 position) override
    {
        const juce::ScopedLock lock(transportLock);
        transport.setNextReadPosition(position);
    }

    juce::int64 getNextReadPosition() const override
    {
        const juce::ScopedLock lock(transportLock);
        return transport.getNextReadPosition();
    }

    juce::int64 getTotalLength() const override
    {
        const juce::ScopedLock lock(transportLock);
        return transport.getTotalLength();
    }

    bool isLooping() const override
    {
        const juce::ScopedLock lock(transportLock);
        return transport.isLooping();
    }

    void setGain(float gain)
    {
        const juce::ScopedLock lock(transportLock);
        transport.setGain(gain);
    }

    float getGain() const
    {
        const juce::ScopedLock lock(transportLock);
        return transport.getGain();
    }

private:
    juce::AudioTransportSource transport;
    mutable juce::CriticalSection transportLock;
    // The low bit is the play state; the other bits invalidate stale callbacks.
    std::atomic<std::uint64_t> playbackState { 0 };
    bool sourceAttached = false;
};
