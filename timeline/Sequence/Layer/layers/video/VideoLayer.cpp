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

	moviePlayer.reset(createVideoPlayer());
	overlapPlayer.reset(createVideoPlayer());
	moviePlayer->addListener(this);
	overlapPlayer->addListener(this);
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
		moviePlayer->unload();
	}
	if (overlapPlayer != nullptr)
	{
		overlapPlayer->stop();
		overlapPlayer->unload();
	}
	loadedClip = nullptr;
	loadedOverlapClip = nullptr;
	pendingLoadClip = nullptr;
	pendingOverlapLoadClip = nullptr;

	settingPlayState = false;

	BaseItem::clearItem();
	clipManager.clear();
	SequenceLayer::clearItem();
}

VideoPlayerEngine* VideoLayer::createVideoPlayer()
{
	return new NullVideoPlayer();
}

VideoLayerClip* VideoLayer::createVideoClip()
{
	return new VideoLayerClip();
}

float VideoLayer::getLocalTime()
{
	if (currentClip == nullptr || currentClip.wasObjectDeleted()) return 0;

	return getLocalTimeForClip(currentClip);
}

float VideoLayer::getLocalTimeForClip(VideoLayerClip* clip) const
{
	if (clip == nullptr || sequence == nullptr) return 0.0f;
	return clip->clipStartOffset->floatValue() + (sequence->currentTime->floatValue() - clip->time->floatValue());
}

float VideoLayer::getClipFadeFactor(VideoLayerClip* clip) const
{
	if (clip == nullptr || sequence == nullptr) return 0.0f;

	const double now = sequence->currentTime->doubleValue();
	float factor = clip->getManualFadeFactor(now);
	const int index = clipManager.items.indexOf(clip);

	if (index > 0)
	{
		VideoLayerClip* previous = dynamic_cast<VideoLayerClip*>(clipManager.items[index - 1]);
		if (previous != nullptr && previous->enabled->boolValue() && previous->getEndTime() > clip->time->doubleValue())
		{
			const double overlap = juce::jmin(previous->getEndTime(), clip->getEndTime()) - clip->time->doubleValue();
			if (overlap > 0.0 && now < clip->time->doubleValue() + overlap)
			{
				const float f = juce::jlimit(0.0f, 1.0f, (float) ((now - clip->time->doubleValue()) / overlap));
				factor *= f * f;
			}
		}
	}

	if (index >= 0 && index + 1 < clipManager.items.size())
	{
		VideoLayerClip* next = dynamic_cast<VideoLayerClip*>(clipManager.items[index + 1]);
		if (next != nullptr && next->enabled->boolValue() && clip->getEndTime() > next->time->doubleValue())
		{
			const double overlapEnd = juce::jmin(clip->getEndTime(), next->getEndTime());
			const double overlap = overlapEnd - next->time->doubleValue();
			if (overlap > 0.0 && now >= next->time->doubleValue())
			{
				const float f = juce::jlimit(0.0f, 1.0f, (float) ((overlapEnd - now) / overlap));
				factor *= f * f;
			}
		}
	}

	return juce::jlimit(0.0f, 1.0f, factor);
}

float VideoLayer::getVolumeFactor()
{
	float factor = volume->floatValue();

	if (currentClip != nullptr && !currentClip.wasObjectDeleted())
		factor *= currentClip->volume->floatValue() * getClipFadeFactor(currentClip);

	return factor;
}

void VideoLayer::applyVolumeToPlayer()
{
	if (moviePlayer == nullptr) return;

	// Change detection keeps engine calls out of the per-position sync storm.
	const float f = getVolumeFactor();
	if (fabsf(f - lastAppliedVolume) >= 0.0005f)
	{
		lastAppliedVolume = f;
		moviePlayer->setVolume(f);
	}

	if (overlapPlayer != nullptr && overlapClip != nullptr && !overlapClip.wasObjectDeleted())
	{
		const float overlapVolume = volume->floatValue() * overlapClip->volume->floatValue() * getClipFadeFactor(overlapClip);
		if (fabsf(overlapVolume - lastAppliedOverlapVolume) >= 0.0005f)
		{
			lastAppliedOverlapVolume = overlapVolume;
			overlapPlayer->setVolume(overlapVolume);
		}
	}
}

void VideoLayer::setVolume(float value)
{
	volume->setValue(value);
	applyVolumeToPlayer();
}

void VideoLayer::updateCurrentClip()
{
	Array<LayerBlock*> active = clipManager.getBlocksAtTime(sequence->currentTime->doubleValue(), false);
	VideoLayerClip* target = active.size() > 0 ? dynamic_cast<VideoLayerClip*>(active[0]) : nullptr;
	VideoLayerClip* overlapTarget = active.size() > 1 ? dynamic_cast<VideoLayerClip*>(active[1]) : nullptr;

	VideoLayerClip* oldCurrent = currentClip.get();
	VideoLayerClip* oldOverlap = overlapClip.get();
	currentClip = target;
	overlapClip = overlapTarget;

	if (oldCurrent != nullptr && oldCurrent != target && oldCurrent != overlapTarget) oldCurrent->isActive->setValue(false);
	if (oldOverlap != nullptr && oldOverlap != target && oldOverlap != overlapTarget) oldOverlap->isActive->setValue(false);
	if (target != nullptr) target->isActive->setValue(true);
	if (overlapTarget != nullptr) overlapTarget->isActive->setValue(true);
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
		moviePlayer->unload();
		loadedClip = nullptr;
		pendingLoadClip = nullptr;
		return;
	}

	if (loadedClip == currentClip
		&& moviePlayer->isFileLoaded()
		&& moviePlayer->getFilePath() == path) return;

	if (pendingLoadClip == currentClip && moviePlayer->getFilePath() == path) return;
	if (lastLoadFailed && lastLoadFailedPath == path) return;

	// Adjacent clips may use the same file after a splice. Keep the decoder and
	// let the player report the new clip as loaded without a black frame.
	if (moviePlayer->isFileLoaded() && moviePlayer->getFilePath() != path)
		moviePlayer->unload();
	loadedClip = nullptr;
	pendingLoadClip = currentClip;
	if (!moviePlayer->load(path))
	{
		pendingLoadClip = nullptr;
		lastLoadFailed = true;
		lastLoadFailedPath = path;
		NLOG(niceName, "Could not load video file : " + path);
	}
}

void VideoLayer::playerFileLoaded(VideoPlayerEngine* player)
{
	const bool isOverlap = player == overlapPlayer.get();
	WeakReference<VideoLayerClip>& pending = isOverlap ? pendingOverlapLoadClip : pendingLoadClip;
	WeakReference<VideoLayerClip>& loaded = isOverlap ? loadedOverlapClip : loadedClip;
	if (pending == nullptr || pending.wasObjectDeleted()) { pending = nullptr; return; }

	VideoLayerClip* clip = pending.get();
	pending = nullptr;
	if (player->getFilePath() != clip->filePath->stringValue()) return;
	loaded = clip;
	lastLoadFailed = false;
	lastLoadFailedPath.clear();
	clip->clipDuration = player->getDuration();
	if (clip->clipDuration <= 0.01f && VideoFileHelpers::isStillImageFile(clip->filePath->stringValue()))
		clip->clipDuration = 10.0f;
	clip->clipLength->setValue((float) clip->clipDuration);
	if (!clip->coreLength->isOverriden)
	{
		clip->coreLength->defaultValue = clip->clipLength->floatValue();
		clip->coreLength->resetValue();
	}
	if (!isOverlap && clipManager.items.size() == 1
		&& totalTimeExpandedForFile != clip->filePath->stringValue()
		&& clip->getTotalLength() > sequence->totalTime->doubleValue()
		&& clip->getTotalLength() > 0)
	{
		totalTimeExpandedForFile = clip->filePath->stringValue();
		clip->time->setValue(0);
		sequence->totalTime->setUndoableValue(sequence->totalTime->doubleValue(), clip->getTotalLength());
	}
	markPlaybackDirty();
}

void VideoLayer::playerFileEnd(VideoPlayerEngine* player)
{
	if (settingPlayState) return;
	const bool isOverlap = player == overlapPlayer.get();
	WeakReference<VideoLayerClip>& pending = isOverlap ? pendingOverlapLoadClip : pendingLoadClip;
	WeakReference<VideoLayerClip>& loaded = isOverlap ? loadedOverlapClip : loadedClip;
	if (pending != nullptr && !player->isFileLoaded())
	{
		if (!pending.wasObjectDeleted())
		{
			lastLoadFailed = true;
			lastLoadFailedPath = pending->filePath->stringValue();
			NLOG(niceName, "Could not load video file : " + lastLoadFailedPath);
		}
		pending = nullptr;
		loaded = nullptr;
		return;
	}
	if (sequence != nullptr && sequence->isPlaying->boolValue() && enabled->boolValue())
	{
		VideoLayerClip* clip = isOverlap ? overlapClip.get() : currentClip.get();
		if (clip != nullptr && getLocalTimeForClip(clip) < clip->clipDuration)
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

void VideoLayer::syncPlaybackState()
{
	if (moviePlayer == nullptr || overlapPlayer == nullptr || sequence == nullptr) return;
	playbackStats.syncCalls++;
	updateCurrentClip();

	// Decoder ownership is changed only here on the message thread. If the
	// outgoing clip just ended, the secondary slot already contains the incoming
	// clip at the correct position, so promote it without reopening the file.
	if (currentClip != nullptr && !currentClip.wasObjectDeleted()
		&& loadedOverlapClip == currentClip && loadedClip != currentClip)
	{
		std::swap(moviePlayer, overlapPlayer);
		std::swap(loadedClip, loadedOverlapClip);
		std::swap(pendingLoadClip, pendingOverlapLoadClip);
		std::swap(lastAppliedVolume, lastAppliedOverlapVolume);
	}

	if (currentClip == nullptr || currentClip.wasObjectDeleted() || currentClip->filePath->stringValue().isEmpty())
	{
		moviePlayer->unload();
		overlapPlayer->unload();
		loadedClip = nullptr;
		loadedOverlapClip = nullptr;
		pendingLoadClip = nullptr;
		pendingOverlapLoadClip = nullptr;
		forceResyncOnPlay = false;
		needsResync = false;
		return;
	}

	if (!enabled->boolValue())
	{
		if (moviePlayer->isPlaying()) moviePlayer->stop();
		if (overlapPlayer->isPlaying()) overlapPlayer->stop();
		return;
	}

	loadCurrentClip();
	if (overlapClip != nullptr && !overlapClip.wasObjectDeleted() && overlapClip->filePath->stringValue().isNotEmpty())
		loadOverlapClip();
	else
	{
		if (overlapPlayer->isFileLoaded() || pendingOverlapLoadClip != nullptr) overlapPlayer->unload();
		loadedOverlapClip = nullptr;
		pendingOverlapLoadClip = nullptr;
		lastAppliedOverlapVolume = -1.0f;
	}

	applyVolumeToPlayer();

	auto syncSlot = [this](VideoPlayerEngine* player, VideoLayerClip* clip, bool forceSeek)
	{
		if (player == nullptr || clip == nullptr || !player->isFileLoaded()) return;
		const float localTime = getLocalTimeForClip(clip);

		if (sequence->isPlaying->boolValue())
		{
			player->setPlaySpeed(sequence->playSpeed->floatValue());
			if (!player->isPlaying() || forceSeek)
			{
				player->setPosition(localTime);
				player->play();
			}
		}
		else
		{
			if (player->isPlaying()) player->pause();
			player->setPosition(localTime);
		}
	};

	const bool forceSeek = forceResyncOnPlay || needsResync;
	syncSlot(moviePlayer.get(), currentClip.get(), forceSeek);
	if (overlapClip != nullptr && !overlapClip.wasObjectDeleted())
		syncSlot(overlapPlayer.get(), overlapClip.get(), forceSeek);

	forceResyncOnPlay = false;
	needsResync = false;
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
	if (clip == nullptr || (clip != currentClip && clip != overlapClip)) return;

	// The throttled sync picks up volume and fade edits while playing.
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

	// Instant transport response : drive the player from this message-thread
	// callback so pause and play hit the player immediately, instead of waiting
	// for the throttled async drag.
	if (moviePlayer == nullptr) return;

	if (currentClip == nullptr || currentClip.wasObjectDeleted() || currentClip->filePath->stringValue().isEmpty())
	{
		moviePlayer->unload();
		loadedClip = nullptr;
		pendingLoadClip = nullptr;
		return;
	}

	loadCurrentClip();

	if (sequence->isPlaying->boolValue())
	{
		if (enabled->boolValue())
		{
			moviePlayer->setPlaySpeed(sequence->playSpeed->floatValue());
			applyVolumeToPlayer();
			if (moviePlayer->isFileLoaded())
			{
				moviePlayer->setPosition(getLocalTime());
				moviePlayer->play();
			}
		}
	}
	else
	{
		if (moviePlayer->isPlaying()) moviePlayer->pause();
		if (moviePlayer->isFileLoaded()) moviePlayer->setPosition(getLocalTime());
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

void VideoLayer::loadOverlapClip()
{
	if (overlapClip == nullptr || overlapClip.wasObjectDeleted() || overlapPlayer == nullptr) return;

#if JUCE_WINDOWS
	if (overlapClip->filePath->stringValue().startsWithChar('/')) return;
#endif

	const String path = overlapClip->filePath->stringValue();
	if (path.isEmpty())
	{
		overlapPlayer->unload();
		loadedOverlapClip = nullptr;
		pendingOverlapLoadClip = nullptr;
		return;
	}

	if (loadedOverlapClip == overlapClip && overlapPlayer->isFileLoaded()
		&& overlapPlayer->getFilePath() == path) return;
	if (pendingOverlapLoadClip == overlapClip && overlapPlayer->getFilePath() == path) return;

	if (overlapPlayer->isFileLoaded() && overlapPlayer->getFilePath() != path)
		overlapPlayer->unload();
	loadedOverlapClip = nullptr;
	pendingOverlapLoadClip = overlapClip;
	if (!overlapPlayer->load(path))
	{
		pendingOverlapLoadClip = nullptr;
		NLOG(niceName, "Could not load overlapping video file : " + path);
	}
}

bool VideoLayer::paste()
{
	if (!clipManager.addItemsFromClipboard(false).isEmpty()) return true;
	return SequenceLayer::paste();
}
