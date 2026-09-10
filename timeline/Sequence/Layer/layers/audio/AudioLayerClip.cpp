/*
  ==============================================================================

	AudioLayerClip.cpp
	Created: 8 Feb 2017 12:20:02pm
	Author:  Ben

  ==============================================================================
*/

#include "JuceHeader.h"
#include "AudioLayerClip.h"

namespace
{
ThreadPool& getAudioClipLoadPool()
{
	// Opening a large project used to create one OS thread per audio clip. A
	// small shared pool keeps file discovery asynchronous without flooding the
	// scheduler and disk when a project contains hundreds of clips.
	static ThreadPool pool(jlimit(1, 4, SystemStats::getNumCpus()));
	return pool;
}
}

AudioLayerClip::AudioLayerClip() :
	LayerBlock(getTypeString()),
	ThreadPoolJob("AudioClipReader"),
	resamplingAudioSource(&channelRemapAudioSource, false),
    channelRemapAudioSource(&transportSource, false),
    clipDuration(0),
	sampleRate(0),
	clipSamplePos(0),
	isLoading(false),
	shouldStop(false),
	audioClipAsyncNotifier(10)
{
	itemDataType = getTypeString();

	filePath = new FileParameter("File Path", "File Path", "");
	addParameter(filePath);

	clipLength = addFloatParameter("Clip Length", "Length of the clip (in seconds)", 0);
	clipLength->defaultUI = FloatParameter::TIME;
	clipLength->setControllableFeedbackOnly(true);
	clipLength->isSavable = false;

	clipStartOffset = addFloatParameter("Clip Start Offset", "Offset at which the clip starts", 0, 0);
	clipStartOffset->defaultUI = FloatParameter::TIME;

	stretchFactor = addFloatParameter("Stretch Factor", "Stretching of  this clip", 1);
	stretchFactor->defaultUI = FloatParameter::TIME;
	stretchFactor->setControllableFeedbackOnly(true);
	stretchFactor->isSavable = false;
	resetStretch = addTrigger("Reset Stretch", "Reset the stretch factor to 1");

	volume = addFloatParameter("Volume", "Volume multiplier", 1, 0);

	fadeIn = addFloatParameter("Fade In", "Fade time at start of the clip", 0, 0);
	fadeIn->defaultUI = FloatParameter::TIME;
	fadeOut = addFloatParameter("Fade Out", "Fade time at end of the clip", 0, 0);
	fadeOut->defaultUI = FloatParameter::TIME;

	formatManager.registerBasicFormats();

}

AudioLayerClip::~AudioLayerClip()
{
	// Jobs are not owned by the pool, so they must be fully removed before the
	// clip and its audio sources are destroyed.
	getAudioClipLoadPool().removeJob(this, true, -1);
	masterReference.clear();
	transportSource.releaseResources();
}

void AudioLayerClip::start()
{
	shouldStop = false;
	transportSource.start();
}

void AudioLayerClip::stop()
{
	shouldStop = true;
}

void AudioLayerClip::updateAudioSourceFile()
{
	if (filePath->stringValue().isEmpty()) return;

#if JUCE_WINDOWS
	if (filePath->stringValue().startsWithChar('/')) return;
#endif

	// Loading while the engine is constructing the project creates heavy thread
	// and disk contention. AudioLayer queues all of its clips from fileLoaded(),
	// once the main object graph is ready.
	if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile) return;

	auto& pool = getAudioClipLoadPool();
	if (pool.contains(this))
		pool.removeJob(this, true, -1);

	pool.addJob(this, false);
}

void AudioLayerClip::prioritizeAudioSourceLoad()
{
	auto& pool = getAudioClipLoadPool();
	if (pool.contains(this)) pool.moveJobToFront(this);
}

void AudioLayerClip::onContainerTriggerTriggered(Trigger* t)
{
	LayerBlock::onContainerTriggerTriggered(t);
	if (t == resetStretch)
	{
		stretchFactor->setValue(1);
		if (coreLength->floatValue() > clipLength->floatValue()) coreLength->setValue(clipLength->floatValue());
	}
}

void AudioLayerClip::onContainerParameterChangedInternal(Parameter* p)
{
	LayerBlock::onContainerParameterChangedInternal(p);
	if (p == filePath)
	{
		updateAudioSourceFile();
	}
	else if (p == isActive)
	{
		if (isActive->boolValue())
		{
			clipSamplePos = 0;
			start();
		}
		else
		{
			stop();
			clipSamplePos = -1;
		}
	}

}

void AudioLayerClip::setCoreLength(float value, bool stretch, bool stickToCoreEnd)
{
	if (stickToCoreEnd)
	{

		float offsetToAdd = value - coreLength->floatValue();
		clipStartOffset->setValue(jlimit<float>(0, (float)clipLength->floatValue() - (float)coreLength->minimumValue, clipStartOffset->floatValue() - offsetToAdd)); //invert to get actual start time of the clip
	}

	if (stretch)
	{
		stretchFactor->setValue(stretchFactor->floatValue() + ((value / coreLength->floatValue()) - 1) * stretchFactor->floatValue());
	}

	LayerBlock::setCoreLength(value, stretch, stickToCoreEnd);
}

void AudioLayerClip::setStartTime(float value, bool stretch, bool stickToCoreEnd)
{
	if (stickToCoreEnd)
	{

	}

	LayerBlock::setStartTime(value, stretch, stickToCoreEnd);

}

void AudioLayerClip::setPlaySpeed(float value)
{
	if (value <= 0) return;
	resamplingAudioSource.setResamplingRatio(stretchFactor->floatValue() / value);
}

void AudioLayerClip::prepareToPlay(int blockSize, int _sampleRate)
{
	resamplingAudioSource.prepareToPlay(blockSize, _sampleRate);
}

ThreadPoolJob::JobStatus AudioLayerClip::runJob()
{
	if (filePath == nullptr || shouldExit()) return jobHasFinished;

	isLoading = true;
	audioClipAsyncNotifier.addMessage(new ClipEvent(ClipEvent::SOURCE_LOAD_START, this));

	transportSource.setSource(nullptr);
	readerSource.reset(nullptr);

	if (!shouldExit()) setupFromSource();

	isLoading = false;
	audioClipAsyncNotifier.addMessage(new ClipEvent(ClipEvent::SOURCE_LOAD_END, this));
	clipListeners.call(&ClipListener::clipSourceLoaded, this);

	return jobHasFinished;
}

void AudioLayerClip::setupFromSource()
{
	AudioFormatReader* reader = formatManager.createReaderFor(filePath->getAbsolutePath());

	if (reader != nullptr)
	{
		std::unique_ptr<AudioFormatReaderSource> newSource(new AudioFormatReaderSource(reader, true));
		transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate, reader->numChannels);
		readerSource.reset(newSource.release());
		sampleRate = reader->sampleRate;
		clipDuration = reader->lengthInSamples / sampleRate;
		numChannels = reader->numChannels;

		clipLength->setValue(clipDuration);
		if (!coreLength->isOverriden)
		{
			coreLength->defaultValue = clipLength->floatValue();
			coreLength->resetValue();
		}

		//buffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
		//reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);
	}
}
