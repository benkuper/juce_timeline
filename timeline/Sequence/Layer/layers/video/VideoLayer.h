/*
  ==============================================================================

    VideoLayer.h
    Created: 26 Sep 2026

  ==============================================================================
*/

#pragma once

#include "VideoPlayerEngine.h"

class VideoLayer :
	public SequenceLayer,
	public VideoLayerClipManager::ManagerListener,
	public VideoLayerClip::ClipListener,
	public VideoPlayerEngine::Listener,
	private juce::AsyncUpdater,
	private juce::Timer
{
public:
	VideoLayer(Sequence* sequence, var params);
	~VideoLayer();

	VideoLayerClipManager clipManager;

	std::unique_ptr<VideoPlayerEngine> moviePlayer;
	std::unique_ptr<VideoPlayerEngine> overlapPlayer;

	WeakReference<VideoLayerClip> currentClip;
	WeakReference<VideoLayerClip> loadedClip;
	WeakReference<VideoLayerClip> pendingLoadClip;
	WeakReference<VideoLayerClip> overlapClip;
	WeakReference<VideoLayerClip> loadedOverlapClip;
	WeakReference<VideoLayerClip> pendingOverlapLoadClip;

	bool settingPlayState;

	// Playback flags: written from the transport thread, applied once on the message thread.
	bool needsResync = false;
	bool forceResyncOnPlay = false;
	bool lastLoadFailed = false;
	String lastLoadFailedPath;

	// Set when the 33ms sync throttle dropped an update : the drain timer guarantees
	// the pending state still gets applied instead of being silently lost.
	bool pendingSync = false;

	juce::uint32 lastResyncTimeMs = 0;
	juce::uint32 lastSyncMs = 0;
	String totalTimeExpandedForFile;

	// Per-instance diagnostics (message thread).
	struct PlaybackStats
	{
		int syncCalls = 0;
		int earlyReturns = 0;
		int playStarts = 0;
		int stopCalls = 0;
		int resyncs = 0;
		juce::int64 costUs = 0;
		juce::int64 maxCostUs = 0;
		juce::uint32 lastLogMs = 0;
		juce::uint32 lastExitLogMs = 0;
		String lastExitLog;
	} playbackStats;

	FloatParameter* volume;

	virtual void clearItem() override;
	virtual VideoPlayerEngine* createVideoPlayer();

	virtual VideoLayerClip* createVideoClip();
	virtual void updateCurrentClip();   // bookkeeping only, safe to call from any thread
	virtual void loadCurrentClip();     // message thread only
	virtual void loadOverlapClip();
	virtual void syncPlaybackState();   // message thread only
	void logSyncExit(const String& why);

	void applyVolumeToPlayer();

	float lastAppliedVolume = -1.0f;
	float lastAppliedOverlapVolume = -1.0f;

	void setSettingPlayState(bool value) { settingPlayState = value; }

	// Marshals playback work to the message thread, safe to call from any thread
	void markPlaybackDirty() { triggerAsyncUpdate(); }

	virtual float getLocalTime();
	float getLocalTimeForClip(VideoLayerClip* clip) const;
	float getClipFadeFactor(VideoLayerClip* clip) const;
	virtual float getVolumeFactor();
	virtual void setVolume(float value);
	void playerFileLoaded(VideoPlayerEngine* player) override;
	void playerFileEnd(VideoPlayerEngine* player) override;
	virtual bool paste() override;

	void itemAdded(LayerBlock* clip) override;
	void itemsAdded(Array<LayerBlock*> clips) override;
	void itemRemoved(LayerBlock* clip) override;
	void itemsRemoved(Array<LayerBlock*> clips) override;

	void clipSourceLoaded(VideoLayerClip* clip) override;
	void clipParamChanged(VideoLayerClip* clip) override;

	void onContainerParameterChangedInternal(Parameter* p) override;
	void onControllableStateChanged(Controllable* c) override;

	virtual SequenceLayerPanel* getPanel() override;
	virtual SequenceLayerTimeline* getTimelineUI() override;

	void sequenceCurrentTimeChanged(Sequence*, float prevTime, bool evaluatedSkippedData) override;
	void sequenceLooped(Sequence*) override;
	void sequencePlayStateChanged(Sequence*) override;

	static VideoLayer* create(Sequence* sequence, var params) { return new VideoLayer(sequence, params); }
	virtual String getTypeString() const override { return "Video"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoLayer)

private:
	void handleAsyncUpdate() override;
	void timerCallback() override;
};
