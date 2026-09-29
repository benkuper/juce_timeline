/*
  ==============================================================================

    VideoLayer.cpp
    Created: 26 Sep 2026

  ==============================================================================
*/

#include "JuceHeader.h"
#include "VideoLayer.h"
#include "VideoFileHelpers.h"

VideoLayer::VideoLayer(Sequence* _sequence, var params) :
	SequenceLayer(_sequence, "Video"),
	clipManager(this),
	settingPlayState(false)
{
	helpID = "VideoLayer";

	volume = addFloatParameter("Volume", "Volume multiplier for the layer", 1, 0, 10);

	clipManager.hideInEditor = true;
	addChildControllableContainer(&clipManager);
	saveAndLoadRecursiveData = true;

	clipManager.addBaseManagerListener(this);

	moviePlayer.reset(new VlcVideoPlayer());

	// All VLC work happens on the message thread : this callback is invoked from
	// the VLC event dispatch (marshalled to the message thread), so it's already there.
	moviePlayer->onPlaybackStopped = [this]()
	{
		if (settingPlayState) return;

		if (sequence != nullptr && sequence->isPlaying->boolValue() && enabled->boolValue())
		{
			// Video reached its end (or was paused) while the sequence is still playing.
			// Only resync while the current position is still inside the clip, and never more
			// often than every 300ms : otherwise a clip that ends while the sequence keeps
			// playing would loop seek→play→end as fast as the message thread can run,
			// freezing the whole interface.
			if (currentClip != nullptr && !currentClip.wasObjectDeleted())
			{
				const float localTime = getLocalTime();

				if (localTime < currentClip->clipDuration)
				{
					const juce::uint32 now = juce::Time::getMillisecondCounter();

					if (now - lastResyncTimeMs > 300)
					{
						lastResyncTimeMs = now;
						needsResync = true;
						markPlaybackDirty();
					}
				}
			}
		}
	};
}

VideoLayer::~VideoLayer()
{
	clearItem();
}

void VideoLayer::clearItem()
{
	settingPlayState = true;

	stopTimer();
	pendingSync = false;

	if (moviePlayer != nullptr)
	{
		moviePlayer->stop();
		moviePlayer->closeVideo();
	}

	settingPlayState = false;

	BaseItem::clearItem();
	clipManager.clear();
	SequenceLayer::clearItem();
}

VideoLayerClip* VideoLayer::createVideoClip()
{
	return new VideoLayerClip();
}

float VideoLayer::getLocalTime()
{
	if (currentClip == nullptr || currentClip.wasObjectDeleted()) return 0;

	return currentClip->clipStartOffset->floatValue() + (sequence->currentTime->floatValue() - currentClip->time->floatValue());
}

float VideoLayer::getVolumeFactor()
{
	float factor = volume->floatValue();

	if (currentClip != nullptr && !currentClip.wasObjectDeleted())
		factor *= currentClip->volume->floatValue();

	return factor;
}

void VideoLayer::applyVolumeToPlayer()
{
	if (moviePlayer == nullptr) return;

	// Always called on the message thread. The change detection keeps the VLC
	// call out of the per-position sync storm : the volume is only pushed to
	// libVLC when it actually changed (layer slider, clip slider, clip switch).
	const float f = getVolumeFactor();
	if (fabsf(f - lastAppliedVolume) < 0.0005f) return;

	lastAppliedVolume = f;
	moviePlayer->setAudioVolume(f);
}

void VideoLayer::applyRenderTransformToPlayer()
{
	if (moviePlayer == nullptr || currentClip == nullptr || currentClip.wasObjectDeleted()) return;

	moviePlayer->setRenderTransform(currentClip->getRenderOpacity(),
		currentClip->getRenderScaleX(),
		currentClip->getRenderScaleY(),
		currentClip->getRenderXPercent(),
		currentClip->getRenderYPercent());
}

void VideoLayer::setVolume(float value)
{
	volume->setValue(value);
	applyVolumeToPlayer();
}

void VideoLayer::updateCurrentClip()
{
	VideoLayerClip* target = nullptr;

	if (!currentClip.wasObjectDeleted() && currentClip != nullptr && currentClip->enabled->boolValue())
	{
		if (currentClip->isInRange(sequence->currentTime->doubleValue())) return;
	}

	target = dynamic_cast<VideoLayerClip*>(clipManager.getBlockAtTime(sequence->currentTime->doubleValue()));

	if (target == currentClip) return;

	if (currentClip != nullptr && !currentClip.wasObjectDeleted())
	{
		currentClip->isActive->setValue(false);
	}

	currentClip = target;

	if (currentClip != nullptr && !currentClip.wasObjectDeleted())
	{
		currentClip->isActive->setValue(true);
	}
}

void VideoLayer::loadCurrentClip()
{
	if (currentClip == nullptr || currentClip.wasObjectDeleted()) return;
	if (moviePlayer == nullptr) return;

#if JUCE_WINDOWS
	if (currentClip->filePath->stringValue().startsWithChar('/')) return;
#endif

	String path = currentClip->filePath->stringValue();

	if (path.isEmpty())
	{
		moviePlayer->closeVideo();
		loadedClip = nullptr;
		return;
	}

	if (loadedClip == currentClip
		&& moviePlayer->isVideoOpen()
		&& moviePlayer->getCurrentVideoFile() == File(path)) return; //already loaded

	if (lastLoadFailed && lastLoadFailedPath == path && loadedClip == currentClip && !moviePlayer->isVideoOpen()) return; //don't retry a failed load

	NLOG(niceName, "loadCurrentClip open due to file change path='" + path + "' loadedClipMatch=" + (loadedClip == currentClip ? "yes" : "no") + " videoOpen=" + (moviePlayer->isVideoOpen() ? "yes" : "no"));

Result r = moviePlayer->load(File(path));

	if (r.wasOk())
	{
		lastLoadFailed = false;
		lastLoadFailedPath = "";

		currentClip->clipDuration = moviePlayer->getVideoDuration();

		// Still images report no length : give them a usable default duration so
		// the block is visible on the timeline and can be looped/held.
		if (currentClip->clipDuration <= 0.01f && VideoFileHelpers::isStillImageFile(path))
		{
			currentClip->clipDuration = 10.0f;
		}

		currentClip->clipLength->setValue((float) currentClip->clipDuration);

		if (!currentClip->coreLength->isOverriden)
		{
			currentClip->coreLength->defaultValue = currentClip->clipLength->floatValue();
			currentClip->coreLength->resetValue();
		}

		loadedClip = currentClip;

		if (!moviePlayer->wasLastLoadReuse())
		{
			NLOG(niceName, "Loaded video '"
				+ currentClip->filePath->stringValue()
				+ "' duration=" + String(currentClip->clipDuration)
				+ " clipLength=" + String(currentClip->clipLength->floatValue())
				+ " coreLength=" + String(currentClip->coreLength->floatValue()));
		}

		if (clipManager.items.size() == 1
			&& totalTimeExpandedForFile != path
			&& currentClip->getTotalLength() > sequence->totalTime->doubleValue()
			&& currentClip->getTotalLength() > 0)
		{
			totalTimeExpandedForFile = path;
			currentClip->time->setValue(0);
			sequence->totalTime->setUndoableValue(sequence->totalTime->doubleValue(), currentClip->getTotalLength());
			NLOG(niceName, "Imported video file is longer than the sequence, expanding total time to match the video file length.");
		}
	}
	else
	{
		String errorMessage = r.getErrorMessage();

		// "Can't create window" is transient (no top-level peer yet) : retry on the next update instead of remembering the failure.
		if (!errorMessage.containsIgnoreCase("window"))
		{
			lastLoadFailed = true;
			lastLoadFailedPath = path;
		}

		loadedClip = nullptr;
		NLOG(niceName, "Could not load video file : " + path + " - " + errorMessage);
	}
}

void VideoLayer::syncPlaybackState()
{
	// Always runs on the message thread (see handleAsyncUpdate)
	if (moviePlayer == nullptr) return;

	PlaybackStats& stats = playbackStats;
	const juce::int64 tickStart = juce::Time::getHighResolutionTicks();
	const juce::int64 ticksPerMs = juce::Time::getHighResolutionTicksPerSecond() / 1000;
	stats.syncCalls++;

	updateCurrentClip();

	if (currentClip == nullptr || currentClip.wasObjectDeleted() || currentClip->filePath->stringValue().isEmpty())
	{
		logSyncExit("syncExit clip=" + String(currentClip != nullptr ? 1 : 0) + " path='" + (currentClip != nullptr ? currentClip->filePath->stringValue() : "") + "'");
		if (moviePlayer->isVideoOpen()) moviePlayer->closeVideo();
		loadedClip = nullptr;
		forceResyncOnPlay = false;
		needsResync = false;
		return;
	}

	if (!enabled->boolValue())
	{
		logSyncExit("syncExit disabled");
		if (moviePlayer->isPlaying()) moviePlayer->stop();
		return;
	}

	loadCurrentClip();

	if (sequence == nullptr) return;

	// Volume and render transform follow the CURRENT clip : apply them on every
	// sync (even the "still playing the same clip" early-return below), otherwise
	// adjusting the layer or clip volume while playing would do nothing. Both
	// apply Volume changes live via internal change detection.
	applyVolumeToPlayer();
	applyRenderTransformToPlayer();

	if (sequence->isPlaying->boolValue())
	{
		if (moviePlayer->isPlaying() && loadedClip == currentClip && !forceResyncOnPlay)
		{
			if (needsResync)
			{
				stats.resyncs++;
				needsResync = false;
				moviePlayer->setPlayPosition(getLocalTime());
				moviePlayer->play();
			}

			stats.earlyReturns++;

			const juce::int64 cost = (juce::Time::getHighResolutionTicks() - tickStart) / ticksPerMs;
			stats.costUs += cost;
			stats.maxCostUs = jmax(stats.maxCostUs, cost);

			const juce::uint32 now = juce::Time::getMillisecondCounter();
			if (now - stats.lastLogMs > 2000)
			{
				stats.lastLogMs = now;
				NLOG(niceName, "sync calls=" + String(stats.syncCalls)
					+ " early=" + String(stats.earlyReturns)
					+ " play=" + String(stats.playStarts)
					+ " stop=" + String(stats.stopCalls)
					+ " resync=" + String(stats.resyncs)
					+ " avgMs=" + String(stats.costUs / jmax(1, stats.syncCalls))
					+ " maxMs=" + String(stats.maxCostUs));
			}

			return; //already playing the right clip, nothing to do
		}

		stats.playStarts++;
		forceResyncOnPlay = false;
		needsResync = false;
		moviePlayer->setPlaySpeed(sequence->playSpeed->floatValue());
		moviePlayer->setPlayPosition(getLocalTime());
		moviePlayer->play();
	}
	else
	{
		stats.stopCalls++;
		const juce::int64 s0 = juce::Time::getHighResolutionTicks();
		if (moviePlayer->isPlaying()) moviePlayer->stop();
		const juce::int64 s1 = juce::Time::getHighResolutionTicks();
		moviePlayer->setPlayPosition(getLocalTime());
		const juce::int64 s2 = juce::Time::getHighResolutionTicks();
		const juce::int64 costMs = (s2 - s0) / ticksPerMs;
		stats.costUs += costMs;
		stats.maxCostUs = jmax(stats.maxCostUs, costMs);
		if (costMs > 10)
			NLOG(niceName, "PAUSE slow: stopMs=" + String((s1 - s0) / ticksPerMs) + " seekMs=" + String((s2 - s1) / ticksPerMs) + " totalMs=" + String(costMs));
	}
}

void VideoLayer::handleAsyncUpdate()
{
	// The throttle coalesces the transport-driven update storm (currentTime ticks
	// at the fps rate). An update that arrives inside the freeze window must never
	// be silently dropped : arm the drain timer to fire exactly when the window
	// opens again (e.g. right after Stop, when nothing else would ever resync).
	const juce::uint32 now = juce::Time::getMillisecondCounter();
	const juce::uint32 sinceLast = now - lastSyncMs;

	if (sinceLast < 33)
	{
		pendingSync = true;
		startTimer((int) (33 - sinceLast + 1));
		return;
	}

	pendingSync = false;
	lastSyncMs = now;
	syncPlaybackState();
}

void VideoLayer::timerCallback()
{
	stopTimer();

	if (!pendingSync) return;

	const juce::uint32 now = juce::Time::getMillisecondCounter();
	const juce::uint32 sinceLast = now - lastSyncMs;

	if (sinceLast < 33)
	{
		startTimer((int) (33 - sinceLast + 1));
		return;
	}

	pendingSync = false;
	lastSyncMs = now;
	syncPlaybackState();
}

void VideoLayer::itemAdded(LayerBlock* clip)
{
	((VideoLayerClip*)clip)->addClipListener(this);

	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) return;
	updateCurrentClip();
	markPlaybackDirty();
}

void VideoLayer::itemsAdded(Array<LayerBlock*> clips)
{
	for (auto& clip : clips)
	{
		((VideoLayerClip*)clip)->addClipListener(this);

		if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) continue;
	}

	updateCurrentClip();
	markPlaybackDirty();
}

void VideoLayer::itemRemoved(LayerBlock* clip)
{
	((VideoLayerClip*)clip)->removeClipListener(this);

	updateCurrentClip();
	markPlaybackDirty();
}

void VideoLayer::itemsRemoved(Array<LayerBlock*> clips)
{
	for (auto& clip : clips)
	{
		((VideoLayerClip*)clip)->removeClipListener(this);
	}

	updateCurrentClip();
	markPlaybackDirty();
}

void VideoLayer::clipSourceLoaded(VideoLayerClip* clip)
{
	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile)
	{
		NLOG(niceName, "clipSourceLoaded SKIPPED (engine loading)");
		return;
	}

	if (clip == nullptr || clip->filePath->stringValue().isEmpty()) return;

	NLOG(niceName, "clipSourceLoaded '" + clip->filePath->stringValue() + "'");

	// Load synchronously instead of raising an async update : the message posted by
	// markPlaybackDirty() right after a file drop is not guaranteed to be delivered before
	// the transport starts, so the block kept its default length until the first Play.
	// updateCurrentClip() can also report no valid clip while the sequence is still being
	// constructed, so fall back to the clip that raised the event.
	if (currentClip == nullptr || currentClip.wasObjectDeleted())
		currentClip = clip;

	loadCurrentClip();

	markPlaybackDirty();
}

void VideoLayer::clipParamChanged(VideoLayerClip* clip)
{
	if (clip == nullptr || clip != currentClip) return;

	// Volume / opacity / transform / blend mode were edited : the throttled sync
	// picks the new volume up immediately (even while playing) and re-pushes the
	// render transform to the preview player.
	markPlaybackDirty();
}

void VideoLayer::onContainerParameterChangedInternal(Parameter* p)
{
	SequenceLayer::onContainerParameterChangedInternal(p);

	if (p == volume)
	{
		markPlaybackDirty();
	}
}

void VideoLayer::onControllableStateChanged(Controllable* c)
{
	SequenceLayer::onControllableStateChanged(c);

	if (c == enabled)
	{
		markPlaybackDirty();
	}
}

SequenceLayerPanel* VideoLayer::getPanel()
{
	return new VideoLayerPanel(this);
}

SequenceLayerTimeline* VideoLayer::getTimelineUI()
{
	return new VideoLayerTimeline(this);
}

void VideoLayer::sequenceCurrentTimeChanged(Sequence* s, float, bool)
{
	updateCurrentClip();
	markPlaybackDirty();

	// A manual seek while playing must move the video along as well : without this,
	// syncPlaybackState's early-return path keeps playing the previous position.
	if (s != nullptr && s->isPlaying->boolValue() && s->isSeeking)
	{
		needsResync = true;
	}
}

void VideoLayer::sequenceLooped(Sequence*)
{
	needsResync = true;
	updateCurrentClip();
	markPlaybackDirty();
}

void VideoLayer::sequencePlayStateChanged(Sequence*)
{
	forceResyncOnPlay = sequence->isPlaying->boolValue();

	updateCurrentClip();
	markPlaybackDirty();

	// Instant transport response : drive libVLC directly from this message-thread
	// callback so pause and play hit the player immediately, instead of waiting
	// for the throttled async drag.
	if (moviePlayer == nullptr) return;

	if (currentClip == nullptr || currentClip.wasObjectDeleted() || currentClip->filePath->stringValue().isEmpty())
	{
		if (moviePlayer->isVideoOpen()) moviePlayer->closeVideo();
		loadedClip = nullptr;
		return;
	}

	loadCurrentClip();

	if (sequence->isPlaying->boolValue())
	{
		if (enabled->boolValue())
		{
			moviePlayer->setPlaySpeed(sequence->playSpeed->floatValue());
			applyVolumeToPlayer();
			moviePlayer->setPlayPosition(getLocalTime());
			moviePlayer->play();
		}
	}
	else
	{
		if (moviePlayer->isPlaying()) moviePlayer->pause();
		moviePlayer->setPlayPosition(getLocalTime());
	}
}

void VideoLayer::logSyncExit(const String& why)
{
	const juce::uint32 now = juce::Time::getMillisecondCounter();
	if (playbackStats.lastExitLog == why && now - playbackStats.lastExitLogMs < 1000) return;
	playbackStats.lastExitLog = why;
	playbackStats.lastExitLogMs = now;
	NLOG(niceName, why);
}
