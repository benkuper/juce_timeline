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
	overlapPlayer.reset(new VlcVideoPlayer());

	moviePlayer->onFrameDecoded = [this](const Image& frame)
	{
		if (currentClip != nullptr && !currentClip.wasObjectDeleted())
			currentClip->cacheThumbnail(getLocalTimeForClip(currentClip), frame);
	};
	overlapPlayer->onFrameDecoded = [this](const Image& frame)
	{
		if (overlapClip != nullptr && !overlapClip.wasObjectDeleted())
			overlapClip->cacheThumbnail(getLocalTimeForClip(overlapClip), frame);
	};

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
	overlapPlayer->onPlaybackStopped = [this]()
	{
		if (!settingPlayState && sequence != nullptr && sequence->isPlaying->boolValue())
		{
			needsResync = true;
			markPlaybackDirty();
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
	if (overlapPlayer != nullptr)
	{
		overlapPlayer->stop();
		overlapPlayer->closeVideo();
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

	// Always called on the message thread. The change detection keeps the VLC
	// call out of the per-position sync storm : the volume is only pushed to
	// libVLC when it actually changed (layer slider, clip slider, clip switch).
	const float f = getVolumeFactor();
	if (fabsf(f - lastAppliedVolume) >= 0.0005f)
	{
		lastAppliedVolume = f;
		moviePlayer->setAudioVolume(f);
	}

	if (overlapPlayer != nullptr && overlapClip != nullptr && !overlapClip.wasObjectDeleted())
	{
		const float overlapVolume = volume->floatValue() * overlapClip->volume->floatValue() * getClipFadeFactor(overlapClip);
		if (fabsf(overlapVolume - lastAppliedOverlapVolume) >= 0.0005f)
		{
			lastAppliedOverlapVolume = overlapVolume;
			overlapPlayer->setAudioVolume(overlapVolume);
		}
	}
}

void VideoLayer::applyRenderTransformToPlayer()
{
	if (moviePlayer == nullptr || currentClip == nullptr || currentClip.wasObjectDeleted()) return;

	moviePlayer->setRenderTransform(currentClip->getRenderOpacity() * getClipFadeFactor(currentClip),
		currentClip->getRenderScaleX(),
		currentClip->getRenderScaleY(),
		currentClip->getRenderXPercent(),
		currentClip->getRenderYPercent());

	if (overlapPlayer != nullptr && overlapClip != nullptr && !overlapClip.wasObjectDeleted())
	{
		overlapPlayer->setRenderTransform(overlapClip->getRenderOpacity() * getClipFadeFactor(overlapClip),
			overlapClip->getRenderScaleX(), overlapClip->getRenderScaleY(),
			overlapClip->getRenderXPercent(), overlapClip->getRenderYPercent());
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
		std::swap(lastAppliedVolume, lastAppliedOverlapVolume);
		moviePlayer->onFrameDecoded = [this](const Image& frame)
		{
			if (currentClip != nullptr && !currentClip.wasObjectDeleted())
				currentClip->cacheThumbnail(getLocalTimeForClip(currentClip), frame);
		};
		overlapPlayer->onFrameDecoded = [this](const Image& frame)
		{
			if (overlapClip != nullptr && !overlapClip.wasObjectDeleted())
				overlapClip->cacheThumbnail(getLocalTimeForClip(overlapClip), frame);
		};
	}

	if (currentClip == nullptr || currentClip.wasObjectDeleted() || currentClip->filePath->stringValue().isEmpty())
	{
		if (moviePlayer->isVideoOpen()) moviePlayer->closeVideo();
		if (overlapPlayer->isVideoOpen()) overlapPlayer->closeVideo();
		loadedClip = nullptr;
		loadedOverlapClip = nullptr;
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
		if (overlapPlayer->isVideoOpen()) overlapPlayer->closeVideo();
		loadedOverlapClip = nullptr;
		lastAppliedOverlapVolume = -1.0f;
	}

	applyVolumeToPlayer();
	applyRenderTransformToPlayer();

	auto syncSlot = [this](VlcVideoPlayer* player, VideoLayerClip* clip, bool forceSeek)
	{
		if (player == nullptr || clip == nullptr || !player->isVideoOpen()) return;
		const float localTime = getLocalTimeForClip(clip);

		if (sequence->isPlaying->boolValue())
		{
			player->setPlaySpeed(sequence->playSpeed->floatValue());
			if (!player->isPlaying() || forceSeek)
			{
				player->setPlayPosition(localTime);
				player->play();
			}
		}
		else
		{
			if (player->isPlaying()) player->pause();
			player->setPlayPosition(localTime);
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

void VideoLayer::loadOverlapClip()
{
	if (overlapClip == nullptr || overlapClip.wasObjectDeleted() || overlapPlayer == nullptr) return;

#if JUCE_WINDOWS
	if (overlapClip->filePath->stringValue().startsWithChar('/')) return;
#endif

	const String path = overlapClip->filePath->stringValue();
	if (path.isEmpty())
	{
		overlapPlayer->closeVideo();
		loadedOverlapClip = nullptr;
		return;
	}

	if (loadedOverlapClip == overlapClip && overlapPlayer->isVideoOpen()
		&& overlapPlayer->getCurrentVideoFile() == File(path)) return;

	Result r = overlapPlayer->load(File(path));
	if (r.failed())
	{
		loadedOverlapClip = nullptr;
		NLOG(niceName, "Could not load overlapping video file : " + path + " - " + r.getErrorMessage());
		return;
	}

	overlapClip->clipDuration = overlapPlayer->getVideoDuration();
	if (overlapClip->clipDuration <= 0.01f && VideoFileHelpers::isStillImageFile(path))
		overlapClip->clipDuration = 10.0f;

	overlapClip->clipLength->setValue((float) overlapClip->clipDuration);
	if (!overlapClip->coreLength->isOverriden)
	{
		overlapClip->coreLength->defaultValue = overlapClip->clipLength->floatValue();
		overlapClip->coreLength->resetValue();
	}

	loadedOverlapClip = overlapClip;
}

bool VideoLayer::paste()
{
	if (!clipManager.addItemsFromClipboard(false).isEmpty()) return true;
	return SequenceLayer::paste();
}
