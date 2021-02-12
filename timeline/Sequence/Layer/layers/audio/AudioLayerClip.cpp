/*
  ==============================================================================

	AudioLayerClip.cpp
	Created: 8 Feb 2017 12:20:02pm
	Author:  Ben

  ==============================================================================
*/

AudioLayerClip::AudioLayerClip() :
	LayerBlock(getTypeString()),
	Thread("AudioClipReader"),
	resamplingAudioSource(&channelRemapAudioSource, false),
    channelRemapAudioSource(&transportSource, false),
    clipDuration(0),
	sampleRate(0),
	clipSamplePos(0),
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

	clipStartOffset = addFloatParameter("Clip Start Offset", "Offset at which the clip starts", 0, 0);
	clipStartOffset->defaultUI = FloatParameter::TIME;

	fadeIn = addFloatParameter("Fade In", "Fade time at start of the clip", 0, 0);
	fadeIn->defaultUI = FloatParameter::TIME;
	fadeOut = addFloatParameter("Fade Out", "Fade time at end of the clip", 0, 0);
	fadeOut->defaultUI = FloatParameter::TIME;

	resetStretch = addTrigger("Reset Stretch", "Reset the stretch factor to 1");

	volume = addFloatParameter("Volume", "Volume multiplier", 1, 0);

	formatManager.registerBasicFormats();

}

AudioLayerClip::~AudioLayerClip()
{
	stopThread(3000);
	masterReference.clear();
	transportSource.releaseResources();
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
		}
		else
		{
			transportSource.stop();
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
	resamplingAudioSource.setResamplingRatio(stretchFactor->floatValue() / value);
}

void AudioLayerClip::prepareToPlay(int blockSize, int _sampleRate)
{
	resamplingAudioSource.prepareToPlay(blockSize, _sampleRate);
}

void AudioLayerClip::run()
{
	if (filePath == nullptr) return;

	isLoading = true;
	audioClipAsyncNotifier.addMessage(new ClipEvent(ClipEvent::SOURCE_LOAD_START, this));

	transportSource.setSource(nullptr);
	readerSource.reset(nullptr);

	setupFromSource();

	isLoading = false;
	audioClipAsyncNotifier.addMessage(new ClipEvent(ClipEvent::SOURCE_LOAD_END, this));
	clipListeners.call(&ClipListener::clipSourceLoaded, this);
}

void AudioLayerClip::setupFromSource()
{
	AudioFormatReader* reader = formatManager.createReaderFor(filePath->getAbsolutePath());

	if (reader != nullptr)
	{
		std::unique_ptr<AudioFormatReaderSource> newSource(new AudioFormatReaderSource(reader, true));
		transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate, 4);
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
}
