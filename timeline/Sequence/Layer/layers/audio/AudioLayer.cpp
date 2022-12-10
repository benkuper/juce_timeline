/*
  ==============================================================================

	AudioLayer.cpp
	Created: 20 Nov 2016 3:08:41pm
	Author:  Ben Kuper

  ==============================================================================
*/

#include "JuceHeader.h"

int AudioLayer::graphIDIncrement = 10;

AudioLayer::AudioLayer(Sequence* _sequence, var params) :
	SequenceLayer(_sequence, "Audio"),
	Thread("Volume interpolation"),
	clipManager(this),
	currentGraph(nullptr),
	currentProcessor(nullptr),
	channelsCC("Channels"),
	enveloppe(nullptr),
	numActiveOutputs(0),
	graphID(0), //was -1 but since 5.2.1, generated warning. Should do otherwise ?
	audioOutputGraphID(2),
	targetVolume(1),
	volumeInterpolationAutomation(nullptr),
	stopAtVolumeInterpolationFinish(false),
	clipIsStopping(false)
{

	helpID = "AudioLayer";

	volume = addFloatParameter("Volume", "Volume multiplier for the layer", 1, 0, 10);
	panning = addFloatParameter("Panning", "Left/Right simple panning", 0, -1, 1);

	enveloppe = addFloatParameter("Enveloppe", "Enveloppe", 0, 0, 1);
	enveloppe->isControllableFeedbackOnly = true;

	addChildControllableContainer(&channelsCC);
	addChildControllableContainer(&clipManager);

	clipManager.addBaseManagerListener(this);

	if (!Engine::mainEngine->isLoadingFile) updateSelectedOutChannels();
}

AudioLayer::~AudioLayer()
{
	clearItem();
}

void AudioLayer::clearItem()
{
	stopThread(1000);

	BaseItem::clearItem();
	setAudioProcessorGraph(nullptr);
	clipManager.clear();
	SequenceLayer::clearItem();
}

void AudioLayer::setAudioProcessorGraph(AudioProcessorGraph* graph, AudioProcessorGraph::NodeID outputGraphID)
{
	if (currentGraph != nullptr)
	{
		currentGraph->removeNode(graphID);
		currentProcessor->clear();
		currentProcessor = nullptr;
		channelsData = channelsCC.getJSONData();

		channelsCC.clear();
	}

	currentGraph = graph;

	if (currentGraph != nullptr)
	{

		auto proc = std::unique_ptr<AudioLayerProcessor>(createAudioLayerProcessor());
		currentProcessor = proc.get();

		graphIDIncrement++;
		graphID = AudioProcessorGraph::NodeID(graphIDIncrement);
		currentGraph->addNode(std::move(proc), graphID);

		int numChannels = currentGraph->getMainBusNumOutputChannels();
		AudioChannelSet channelSet = currentGraph->getChannelLayoutOfBus(false, 0);
		for (int i = 0; i < numChannels; ++i)
		{
			String channelName = AudioChannelSet::getChannelTypeName(channelSet.getTypeOfChannel(i));
			BoolParameter* b = channelsCC.addBoolParameter("Channel Out : " + channelName, "If enabled, sends audio from this layer to this channel", false);
			b->setValue(i < 2, false);
		}
	}

	channelsCC.loadJSONData(channelsData);

	audioOutputGraphID = outputGraphID;

	updateSelectedOutChannels();
}

AudioLayerProcessor* AudioLayer::createAudioLayerProcessor()
{
	return new AudioLayerProcessor(this);
}

AudioLayerClip* AudioLayer::createAudioClip()
{
	return new AudioLayerClip();
}


void AudioLayer::updateCurrentClip()
{
	AudioLayerClip* target = nullptr;

	if (sequence->currentTime->floatValue() > 0 || sequence->isPlaying->boolValue()) // only find a clip if sequence not at 0 or is playing
	{
		if (!currentClip.wasObjectDeleted() && currentClip != nullptr && currentClip->isInRange(sequence->currentTime->floatValue())) return;
		target = dynamic_cast<AudioLayerClip*>(clipManager.getBlockAtTime(sequence->currentTime->floatValue()));
	}

	if (target == currentClip) return;

	if (currentClip != nullptr)
	{
		currentClip->isActive->setValue(false);
	}

	currentClip = target;

	if (currentClip != nullptr)
	{
		currentClip->isActive->setValue(true);
		float pos = currentClip->clipStartOffset->floatValue() + (sequence->hiResAudioTime - currentClip->time->floatValue()) / currentClip->stretchFactor->floatValue();
		currentClip->transportSource.setPosition(pos);

		if (sequence->isPlaying->boolValue()) currentClip->start();
		updateSelectedOutChannels();
	}

}

void AudioLayer::itemAdded(LayerBlock* item)
{
	((AudioLayerClip*)item)->addClipListener(this);

	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) return;
	updateClipConfig((AudioLayerClip*)item, true);
	updateSelectedOutChannels();
}

void AudioLayer::itemRemoved(LayerBlock* item)
{
	((AudioLayerClip*)item)->removeClipListener(this);

	updateCurrentClip();
}

void AudioLayer::clipSourceLoaded(AudioLayerClip* clip)
{
	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) return;
	if (clipManager.items.size() == 1 && clip->getTotalLength() > sequence->totalTime->floatValue())
	{
		clip->time->setValue(0);
		sequence->totalTime->setUndoableValue(sequence->totalTime->floatValue(), clip->getTotalLength());
		NLOG(niceName, "Imported audio file is longer than the sequence, expanding total time to match the audio file length.");
	}
}

void AudioLayer::updateSelectedOutChannels()
{

	selectedOutChannels.clear();

	if (currentGraph == nullptr) return;

	selectedOutChannels.clear();
	for (int i = 0; i < channelsCC.controllables.size(); ++i) if (((BoolParameter*)channelsCC.controllables[i])->boolValue()) selectedOutChannels.add(i);

	numActiveOutputs = selectedOutChannels.size();

	currentGraph->disconnectNode(graphID);

	currentProcessor->setPlayConfigDetails(0, numActiveOutputs, currentGraph->getSampleRate(), currentGraph->getBlockSize());
	currentProcessor->prepareToPlay(currentGraph->getSampleRate(), currentGraph->getBlockSize());

	for (auto& c : clipManager.items)
	{
		updateClipConfig((AudioLayerClip*)c, false);
	}

	for (auto& c : clipManager.items) ((AudioLayerClip*)c)->channelRemapAudioSource.setNumberOfChannelsToProduce(numActiveOutputs);

	for (int i = 0; i < numActiveOutputs; ++i)
	{
		currentGraph->addConnection({ {graphID, i }, {audioOutputGraphID, selectedOutChannels[i]} });

		for (auto& c : clipManager.items)
		{
			((AudioLayerClip*)c)->channelRemapAudioSource.setOutputChannelMapping(i, i);
		}
	}
}

void AudioLayer::updateClipConfig(AudioLayerClip* clip, bool updateOutputChannelRemapping)
{
	if (clip == nullptr) return;

	clip->channelRemapAudioSource.clearAllMappings();
	//clip->channelRemapAudioSource.prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
	clip->setPlaySpeed(sequence->playSpeed->floatValue());
	if (currentGraph != nullptr)	clip->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());

	if (updateOutputChannelRemapping)
	{
		int index = 0;
		for (int i = 0; i < channelsCC.controllables.size(); ++i)
		{
			if (((BoolParameter*)channelsCC.controllables[i])->boolValue())
			{
				clip->channelRemapAudioSource.setOutputChannelMapping(index, index);
			}
			index++;
		}

		clip->channelRemapAudioSource.setNumberOfChannelsToProduce(numActiveOutputs);
	}

}

float AudioLayer::getVolumeFactor()
{
	return volume->floatValue();// * layer->audioModule->outVolume->floatValue()
}

void AudioLayer::setVolume(float value, float time, Automation* automation, bool stopSequenceAtFinish)
{
	if (time == 0)
	{
		volume->setValue(value);
		if (stopSequenceAtFinish) sequence->stopTrigger->trigger();
		return;
	}

	stopThread(1000);


	targetVolume = value;
	if (volumeInterpolationAutomation != nullptr) volumeInterpolationAutomation->removeInspectableListener(this);
	volumeInterpolationAutomation = automation;
	volumeAutomationRef = volume;
	if (volumeInterpolationAutomation != nullptr) volumeInterpolationAutomation->addInspectableListener(this);

	volumeInterpolationTime = time;
	stopAtVolumeInterpolationFinish = stopSequenceAtFinish;

	startThread();
}

void AudioLayer::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	SequenceLayer::onControllableFeedbackUpdateInternal(cc, c);

	if (cc == &channelsCC)
	{
		if (!isCurrentlyLoadingData) updateSelectedOutChannels();
	}
	else if (currentClip != nullptr)
	{
		if (c == currentClip->enabled) updateCurrentClip();
		else if (c == currentClip->time || c == currentClip->stretchFactor)
		{
			float pos = currentClip->clipStartOffset->floatValue() + (sequence->hiResAudioTime - currentClip->time->floatValue()) / currentClip->stretchFactor->floatValue();
			currentClip->transportSource.setPosition(pos);

			if (c == currentClip->stretchFactor)
			{
				currentClip->setPlaySpeed(sequence->playSpeed->floatValue());
				currentClip->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
			}
		}
	}
}

void AudioLayer::selectAll(bool addToSelection)
{
	clipManager.askForSelectAllItems(addToSelection);
	setSelected(false);
}

var AudioLayer::getJSONData()
{
	var data = SequenceLayer::getJSONData();
	data.getDynamicObject()->setProperty(clipManager.shortName, clipManager.getJSONData());
	if (currentGraph != nullptr)
	{
		data.getDynamicObject()->setProperty("channels", channelsCC.getJSONData());
	}

	return data;
}

void AudioLayer::loadJSONDataInternal(var data)
{
	channelsCC.loadJSONData(data.getProperty("channels", var()));
	SequenceLayer::loadJSONDataInternal(data);
	clipManager.loadJSONData(data.getProperty(clipManager.shortName, var()));
	updateSelectedOutChannels();
}

SequenceLayerPanel* AudioLayer::getPanel()
{
	return new AudioLayerPanel(this);
}

SequenceLayerTimeline* AudioLayer::getTimelineUI()
{
	return new AudioLayerTimeline(this);
}

void AudioLayer::sequenceCurrentTimeChanged(Sequence*, float, bool)
{
	if (enveloppe == nullptr) return;

	if (currentProcessor != nullptr) enveloppe->setValue(currentProcessor->currentEnveloppe);
	else enveloppe->setValue(0);

	updateCurrentClip();

	if (currentClip != nullptr && sequence->isSeeking)
	{
		float pos = currentClip->clipStartOffset->floatValue() + (sequence->hiResAudioTime - currentClip->time->floatValue()) / currentClip->stretchFactor->floatValue();
		currentClip->transportSource.setPosition(pos);
	}
}

void AudioLayer::sequencePlayStateChanged(Sequence*)
{
	if (!sequence->isPlaying->boolValue())
	{
		enveloppe->setValue(0);
		if (currentClip != nullptr)
		{
			clipIsStopping = true;
			currentClip->transportSource.stop();
			clipIsStopping = false;
		}
	}
	else
	{
		if (currentClip != nullptr && enabled->boolValue())
		{
			float pos = currentClip->clipStartOffset->floatValue() + (sequence->hiResAudioTime - currentClip->time->floatValue()) / currentClip->stretchFactor->floatValue();
			currentClip->transportSource.setPosition(pos);
			currentClip->start();
		}
	}
}

void AudioLayer::sequencePlaySpeedChanged(Sequence*)
{
	if (currentClip != nullptr)
	{
		currentClip->setPlaySpeed(sequence->playSpeed->floatValue());
		currentClip->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
	}
}

void AudioLayer::sequencePlayDirectionChanged(Sequence*)
{
	if (currentClip != nullptr)
	{
		float pos = currentClip->clipStartOffset->floatValue() + (sequence->hiResAudioTime - currentClip->time->floatValue()) / currentClip->stretchFactor->floatValue();
		currentClip->transportSource.setPosition(pos);
	}
}

void AudioLayer::getSnapTimes(Array<float>* arrayToFill)
{
	return clipManager.getSnapTimes(arrayToFill);
}

void AudioLayer::run()
{
	if (volumeInterpolationAutomation == nullptr || volumeInterpolationTime <= 0) return;

	Automation a;
	a.isSelectable = false;
	a.hideInEditor = true;
	a.loadJSONData(volumeInterpolationAutomation->getJSONData());

	float volumeAtStart = volume->floatValue();
	double timeAtStart = Time::getMillisecondCounter() / 1000.0;

	while (!threadShouldExit() && !volumeAutomationRef.wasObjectDeleted())
	{
		double curTime = Time::getMillisecondCounter() / 1000.0;
		double rel = jlimit<double>(0., 1., (curTime - timeAtStart) / volumeInterpolationTime);

		float weight = volumeInterpolationAutomation->getValueAtPosition(rel);
		volume->setValue(jmap(weight, volumeAtStart, targetVolume));

		if (rel == 1) break;

		wait(20); //50fps
	}

	if (volumeInterpolationAutomation != nullptr)
	{
		volumeInterpolationAutomation->removeInspectableListener(this);
		volumeInterpolationAutomation = nullptr;
		volumeAutomationRef = nullptr;
	}

	if (stopAtVolumeInterpolationFinish) sequence->stopTrigger->trigger();

}

void AudioLayer::inspectableDestroyed(Inspectable* i)
{
	if (i == volumeInterpolationAutomation)
	{
		stopThread(1000);
	}
}


//Audio Processor

AudioLayerProcessor::AudioLayerProcessor(AudioLayer* _layer) :
	layer(_layer),
	rmsCount(0),
	tempRMS(0),
	currentEnveloppe(0)
{
}

AudioLayerProcessor::~AudioLayerProcessor()
{
	layer = nullptr;
}

void AudioLayerProcessor::clear()
{
	layer = nullptr;
}

const String AudioLayerProcessor::getName() const
{
	return String("Sequence AudioLayer");
}

void AudioLayerProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
}

void AudioLayerProcessor::releaseResources()
{

}

void AudioLayerProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	bool noProcess = false;

	WeakReference<AudioLayerClip> currentClip;

	if (layer != nullptr)
	{
		if (!layer->enabled->boolValue()
			|| !layer->sequence->enabled->boolValue()
			|| !layer->sequence->isPlaying->boolValue()
			|| layer->sequence->playSpeed->floatValue() < 0) noProcess = true;

		currentClip = layer->currentClip;

		if (currentClip.wasObjectDeleted() || currentClip.get() == nullptr) noProcess = true;
		else if (currentClip->filePath->stringValue().isEmpty()) noProcess = true;

	}
	else
	{
		noProcess = true;
	}

	if (buffer.getNumChannels() == 0) noProcess = true;

	//Do this before noProcess to avoid freeze of transportSource when stopping
	AudioSourceChannelInfo bufferToFill;
	bufferToFill.buffer = &buffer;
	bufferToFill.startSample = 0;
	bufferToFill.numSamples = buffer.getNumSamples();


	if (currentClip != nullptr)
	{
		if (currentClip->shouldStop) currentClip->transportSource.stop();
		if ((!noProcess || currentClip->transportSource.isPlaying() || layer->clipIsStopping))
		{
			currentClip->channelRemapAudioSource.getNextAudioBlock(bufferToFill);
		}
	}

	if (noProcess)
	{
		currentEnveloppe = 0;
		rmsCount = 0;
		tempRMS = 0;
		buffer.clear();
		return;
	}

	float currentTime = layer->sequence->currentTime->floatValue();
	float relClipStart = currentTime - currentClip->time->floatValue();
	float relClipEnd = currentClip->getEndTime() - currentTime;

	float clipVolume = currentClip->volume->controlMode == Parameter::ControlMode::AUTOMATION ? ((Automation*)currentClip->volume->automation->automationContainer)->getValueAtPosition(relClipStart) : currentClip->volume->floatValue();
	float volumeFactor = clipVolume * layer->getVolumeFactor();


	if (relClipStart < currentClip->fadeIn->floatValue())
	{
		float fadeIn = relClipStart / currentClip->fadeIn->floatValue();
		volumeFactor *= fadeIn * fadeIn; //square to have an ease InOut
	}

	if (relClipEnd < currentClip->fadeOut->floatValue())
	{
		float fadeOut = relClipEnd / currentClip->fadeOut->floatValue();
		volumeFactor *= fadeOut * fadeOut;
	}

	//layer->currentClip->transportSource.setGain(volumeFactor);
	buffer.applyGain(volumeFactor);

	if (buffer.getNumChannels() >= 2)
	{
		float panning = layer->panning->floatValue();
		if (panning < 0) buffer.applyGain(1, bufferToFill.startSample, bufferToFill.numSamples, 1 + panning);
		else if (panning > 0) buffer.applyGain(0, bufferToFill.startSample, bufferToFill.numSamples, 1 - panning);
	}

	float rms = 0;
	for (int i = 0; i < buffer.getNumChannels(); ++i)
	{
		float rmsLevel = buffer.getRMSLevel(i, bufferToFill.startSample, bufferToFill.numSamples);
		rms = jmax(rms, rmsLevel);
	}

	tempRMS += rms;
	rmsCount++;
	if (rmsCount * bufferToFill.numSamples >= minEnveloppeSamples)
	{
		currentEnveloppe = tempRMS / rmsCount;
		rmsCount = 0;
		tempRMS = 0;
	}


}

double AudioLayerProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

bool AudioLayerProcessor::acceptsMidi() const
{
	return false;
}

bool AudioLayerProcessor::producesMidi() const
{
	return false;
}

AudioProcessorEditor* AudioLayerProcessor::createEditor()
{
	return nullptr;
}

bool AudioLayerProcessor::hasEditor() const
{
	return false;
}

int AudioLayerProcessor::getNumPrograms()
{
	return 0;
}

int AudioLayerProcessor::getCurrentProgram()
{
	return 0;
}

void AudioLayerProcessor::setCurrentProgram(int index)
{
}

const String AudioLayerProcessor::getProgramName(int index)
{
	return String();
}

void AudioLayerProcessor::changeProgramName(int index, const String& newName)
{
}

void AudioLayerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void AudioLayerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
}
