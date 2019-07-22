#include "AudioLayerClip.h"
/*
  ==============================================================================

	AudioLayerClip.cpp
	Created: 8 Feb 2017 12:20:02pm
	Author:  Ben

  ==============================================================================
*/

AudioLayerClip::AudioLayerClip(float _time) :
	LayerBlock("AudioClip"),
	Thread("AudioClipReader"),
	channelRemapAudioSource(&transportSource, false),
	resamplingAudioSource(&channelRemapAudioSource, false),
	clipDuration(0),
	sampleRate(0),
	clipSamplePos(0),
	clipStartOffset(0),
	isCurrent(false),
	isLoading(false),

	audioClipAsyncNotifier(10)
{
	itemDataType = "AudioClip";

	filePath = new FileParameter("File Path", "File Path", "");
	addParameter(filePath);

	clipLength = addFloatParameter("Clip Length", "Length of the clip (in seconds)", 0);
	clipLength->defaultUI = FloatParameter::TIME;
	clipLength->setControllableFeedbackOnly(true);
	clipLength->isSavable = false;

	stretchFactor = addFloatParameter("Stretch Factor", "Stretching of  this clip", 1);
	stretchFactor->defaultUI = FloatParameter::TIME;
	stretchFactor->setControllableFeedbackOnly(true);
	stretchFactor->isSavable = false;

	resetStretch = addTrigger("Reset Stretch", "Reset the stretch factor to 1");

	volume = addFloatParameter("Volume", "Volume multiplier", 1, 0, 50);

	isLocked = addBoolParameter("Locked", "When locked, you can't change time or flag values", false);


	formatManager.registerBasicFormats();

}

AudioLayerClip::~AudioLayerClip()
{
	signalThreadShouldExit();
	waitForThreadToExit(3000);
	masterReference.clear();
	transportSource.releaseResources();
}

void AudioLayerClip::setIsCurrent(bool value)
{
	if (isCurrent == value) return;
	isCurrent = value;

	if (isCurrent)
	{
		clipSamplePos = 0;
	} else
	{
		transportSource.stop();
		clipSamplePos = -1;
	}

	clipListeners.call(&ClipListener::clipIsCurrentChanged, this);
	audioClipAsyncNotifier.addMessage(new ClipEvent(ClipEvent::CLIP_IS_CURRENT_CHANGED, this));
}

bool AudioLayerClip::isInRange(float _time)
{
	return (_time >= time->floatValue() && _time <= getEndTime());
}

void AudioLayerClip::updateAudioSourceFile()
{
	if (filePath->stringValue().isEmpty()) return;

#if JUCE_WINDOWS
	if (filePath->stringValue().startsWithChar('/')) return;
#endif

	startThread();
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

void AudioLayerClip::onContainerParameterChangedInternal(Parameter * p)
{
	LayerBlock::onContainerParameterChangedInternal(p);
	if (p == filePath)
	{
		updateAudioSourceFile();
	}

}

void AudioLayerClip::setCoreLength(float value, bool stretch, bool stickToCoreEnd)
{
	if (stickToCoreEnd)
	{

		float offsetToAdd = value - coreLength->floatValue();
		clipStartOffset = jlimit<float>(0, (float)clipLength->floatValue() - (float)coreLength->minimumValue, clipStartOffset - offsetToAdd); //invert to get actual start time of the clip
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
	resamplingAudioSource.setResamplingRatio(stretchFactor->floatValue() / value);
}

void AudioLayerClip::run()
{
	if (filePath == nullptr) return;

	isLoading = true;
	audioClipAsyncNotifier.addMessage(new ClipEvent(ClipEvent::SOURCE_LOAD_START, this));

	transportSource.setSource(nullptr);
	readerSource.reset(nullptr);

	AudioFormatReader * reader = formatManager.createReaderFor(filePath->getAbsolutePath());

	if (reader != nullptr)
	{
		std::unique_ptr<AudioFormatReaderSource> newSource(new AudioFormatReaderSource(reader, true));
		transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
		readerSource.reset(newSource.release());
		sampleRate = reader->sampleRate;
		clipDuration = reader->lengthInSamples / sampleRate;

		clipLength->setValue(clipDuration);
		if (!coreLength->isOverriden)
		{
			coreLength->defaultValue = clipLength->floatValue();
			coreLength->resetValue();
		}

		//buffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
		//reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);
	}

	isLoading = false;
	audioClipAsyncNotifier.addMessage(new ClipEvent(ClipEvent::SOURCE_LOAD_END, this));
	clipListeners.call(&ClipListener::clipSourceLoaded, this);
}
