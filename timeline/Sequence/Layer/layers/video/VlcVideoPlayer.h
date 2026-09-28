/*
  ==============================================================================

    VlcVideoPlayer.h
    Created: 27 Sep 2026

    Video playback backend based on libVLC (dynamic loading), used on Windows
    instead of the DirectShow-based juce::VideoComponent.

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

#include <atomic>
#include <functional>

class VlcVideoPlayer :
	public Component,
	private AsyncUpdater
{
public:
	VlcVideoPlayer();
	~VlcVideoPlayer();

	Result load(const File& file);
	void closeVideo();
	bool isVideoOpen() const;
	File getCurrentVideoFile() const;
	bool wasLastLoadReuse() const { return lastLoadWasReuse; }

	double getVideoDuration() const;
	double getPlayPosition() const;
	void setPlayPosition(double seconds);
	void setPlaySpeed(double rate);
	void setAudioVolume(float volume);

	void play();
	void pause();
	void stop();
	bool isPlaying() const;

	// Latest decoded frame, useful for compositing this player with other layers.
	// Returns a default (invalid) Image until the first frame has been decoded.
	juce::Image getCurrentFrame() const;

	// Per-clip render state used by the preview paint : opacity + scale + offset.
	void setRenderTransform(float opacity, float scaleX, float scaleY, float xPercent, float yPercent);

	// Draws a frame fitted into targetArea, scaled/offset as in the Transform
	// parameters and with the given opacity. Shared by the preview player and the
	// Composition Video window so both apply a clip the same way.
	static void drawFrameWithTransform(juce::Graphics& g, const juce::Image& img, juce::Rectangle<int> targetArea, float opacity, float scaleX, float scaleY, float xPercent, float yPercent);

	std::function<void()> onPlaybackStopped;

	void paint(Graphics& g) override;

private:
	struct FrameStore;
	std::unique_ptr<FrameStore> frameStore;

	void* vlcInstance = nullptr;
	void* media = nullptr;
	void* player = nullptr;
	void* eventManager = nullptr;

	struct RenderState
	{
		float opacity = 1.0f;
		float scaleX = 1.0f;
		float scaleY = 1.0f;
		float xPct = 0.0f;
		float yPct = 0.0f;
	};
	RenderState renderState;

	File currentFile;
	bool videoLoaded = false;
	bool lastLoadWasReuse = false;

	juce::int64 durationMs = 0;
	std::atomic<bool> hasNewFrame { false };
	std::atomic<bool> needsStoppedEvent { false };

	void handlePlaybackStoppedEvent();
	void handleAsyncUpdate() override;

	friend void* vlcLock(void*, void**);
	friend void vlcUnlock(void*, void*, void* const*);
	friend void vlcDisplay(void*, void*);
	friend unsigned vlcFormatSetup(void**, char*, unsigned*, unsigned*, unsigned*, unsigned*);
	friend void vlcCleanup(void*);
	friend void vlcEventCallback(const void*, void*);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VlcVideoPlayer)
};