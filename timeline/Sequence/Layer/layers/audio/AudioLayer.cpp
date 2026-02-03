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
	numActiveInputs(0),
	numActiveOutputs(0),
	graphID(0), //was -1 but since 5.2.1, generated warning. Should do otherwise ?
	audioOutputGraphID(2),
	targetVolume(1),
	volumeInterpolationAutomation(nullptr),
	stopAtVolumeInterpolationFinish(false),
	clipIsStopping(false),
	metronomeCC("Metronome Channels"),
	prevMetronomeBeat(0),
	settingAudioGraph(false)
{

	helpID = "AudioLayer";

	volume = addFloatParameter("Volume", "Volume multiplier for the layer", 1, 0, 10);
	panning = addFloatParameter("Panning", "Left/Right simple panning", 0, -1, 1);

	enveloppe = addFloatParameter("Enveloppe", "Enveloppe", 0, 0, 1);
	enveloppe->isControllableFeedbackOnly = true;

	addChildControllableContainer(&channelsCC);

	routeMonoToAllChannels = addBoolParameter("Route Mono to all channels", "If the audio is mono, route it to all selected output channels", true);

	metronomeVolume = addFloatParameter("Metronome", "Enable and control metronome volume here", 1, 0, 5, false);
	metronomeVolume->canBeDisabledByUser = true;

	bip1File = addFileParameter("Custom Bip", "Custom 1st beat sound file for metronome", "", false);
	bip2File = addFileParameter("Custom Bop", "Custom other beats sound file for metronome", "", false);
	bip1File->canBeDisabledByUser = true;
	bip2File->canBeDisabledByUser = true;

	addChildControllableContainer(&metronomeCC);


	clipManager.hideInEditor = true;
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
	settingAudioGraph = true;

	if (currentGraph != nullptr)
	{
		currentGraph->removeNode(graphID);
		currentProcessor->clear();
		currentProcessor = nullptr;
		channelsData = channelsCC.getJSONData();
		metronomeData = metronomeCC.getJSONData();

		channelsCC.clear();
		metronomeCC.clear();
	}

	currentGraph = graph;

	if (currentGraph != nullptr)
	{

		auto proc = std::unique_ptr<AudioLayerProcessor>(createAudioLayerProcessor());
		currentProcessor = proc.get();

		int graphIDInc = getNodeGraphIDIncrement();
		graphID = AudioProcessorGraph::NodeID(graphIDInc);
		currentGraph->addNode(std::move(proc), graphID);

		int numChannels = currentGraph->getMainBusNumOutputChannels();
		AudioChannelSet channelSet = currentGraph->getChannelLayoutOfBus(false, 0);
		for (int i = 0; i < numChannels; ++i)
		{
			String channelName = AudioChannelSet::getChannelTypeName(channelSet.getTypeOfChannel(i));

			BoolParameter* b = channelsCC.addBoolParameter("Channel " + String(i + 1) + " : " + channelName, "If enabled, sends audio from this layer to this channel", false);
			b->setValue(i < 2, false);

			BoolParameter* mb = metronomeCC.addBoolParameter("Channel " + String(i + 1) + " : " + channelName, "If enabled, sends audio from this layer to this channel", false);
			mb->setValue(i < 2, false);
		}
	}

	channelsCC.loadJSONData(channelsData);
	metronomeCC.loadJSONData(metronomeData);

	audioOutputGraphID = outputGraphID;

	updateSelectedOutChannels();

	settingAudioGraph = false;

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

	if (sequence->currentTime->doubleValue() > 0 || sequence->isPlaying->boolValue()) // only find a clip if sequence not at 0 or is playing
	{
		if (!currentClip.wasObjectDeleted() && currentClip != nullptr && currentClip->isInRange(sequence->currentTime->doubleValue())) return;
		target = dynamic_cast<AudioLayerClip*>(clipManager.getBlockAtTime(sequence->currentTime->doubleValue()));
	}

	if (target == currentClip) return;

	if (currentClip != nullptr)
	{
		currentClip->isActive->setValue(false);
	}

	currentClip = target;

	if (currentClip != nullptr && !currentClip.wasObjectDeleted())
	{
		currentClip->isActive->setValue(true);
		float pos = currentClip->clipStartOffset->doubleValue() + (sequence->hiResAudioTime - currentClip->time->doubleValue()) / currentClip->stretchFactor->doubleValue();
		currentClip->transportSource.setPosition(pos);

		if (sequence->isPlaying->boolValue()) currentClip->start();
		//updateSelectedOutChannels();
	}

}

void AudioLayer::itemAdded(LayerBlock* item)
{
	((AudioLayerClip*)item)->addClipListener(this);

	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) return;
	updateClipConfig((AudioLayerClip*)item, true);
	updateSelectedOutChannels();
}

void AudioLayer::itemsAdded(Array<LayerBlock*> clips)
{
	for (auto& clip : clips)
	{
		((AudioLayerClip*)clip)->addClipListener(this);

		if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) continue;
		updateClipConfig((AudioLayerClip*)clip, true);
	}

	updateSelectedOutChannels();
}

void AudioLayer::itemRemoved(LayerBlock* item)
{
	((AudioLayerClip*)item)->removeClipListener(this);

	updateCurrentClip();
}

void AudioLayer::itemsRemoved(Array<LayerBlock*> clips)
{
	for (auto& clip : clips)
	{
		((AudioLayerClip*)clip)->removeClipListener(this);
	}

	updateCurrentClip();
}

void AudioLayer::clipSourceLoaded(AudioLayerClip* clip)
{
	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) return;
	if (clipManager.items.size() == 1 && clip->getTotalLength() > sequence->totalTime->doubleValue())
	{
		clip->time->setValue(0);
		sequence->totalTime->setUndoableValue(sequence->totalTime->doubleValue(), clip->getTotalLength());
		NLOG(niceName, "Imported audio file is longer than the sequence, expanding total time to match the audio file length.");
	}
}

void AudioLayer::updateSelectedOutChannels()
{
	if (Engine::mainEngine->isLoadingFile || isCurrentlyLoadingData) return;

	selectedOutChannels.clear();
	metronomeOutChannels.clear();

	clipLocalChannels.clear();
	metronomeLocalChannels.clear();

	if (currentGraph == nullptr) return;

	int numChannelsUsed = 0;


	for (int i = 0; i < channelsCC.controllables.size(); ++i)
	{
		bool chUsed = false;
		if (((BoolParameter*)channelsCC.controllables[i])->boolValue()) {
			selectedOutChannels.add(i);
			clipLocalChannels.add(numChannelsUsed);
			chUsed = true;
		}

		if (metronome != nullptr)
		{
			if (metronomeCC.controllables[i])
				if (((BoolParameter*)metronomeCC.controllables[i])->boolValue())
				{
					metronomeOutChannels.add(i);
					metronomeLocalChannels.add(numChannelsUsed);
					chUsed = true;
				}
		}

		if (chUsed) numChannelsUsed++;
	}

	numActiveOutputs = numChannelsUsed;

	currentGraph->disconnectNode(graphID);

	updateSelectedOutChannelsInternal();

	updatePlayConfigDetails();

	for (auto& c : clipManager.items)
	{
		updateClipConfig((AudioLayerClip*)c, false);
	}

	for (auto& c : clipManager.items) ((AudioLayerClip*)c)->channelRemapAudioSource.setNumberOfChannelsToProduce(numChannelsUsed);

	if (metronome != nullptr)
	{
		for (auto& c : metronome->ticChannelRemap)
		{
			c->clearAllMappings();
			c->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
			c->setNumberOfChannelsToProduce(numChannelsUsed);
		}
	}

	for (int i = 0; i < selectedOutChannels.size(); ++i)
	{
		currentGraph->addConnection({ {graphID, clipLocalChannels[i]}, {audioOutputGraphID, selectedOutChannels[i]} });

		for (auto& c : clipManager.items)
		{
			((AudioLayerClip*)c)->channelRemapAudioSource.setOutputChannelMapping(i, clipLocalChannels[i]);
		}
	}

	for (int i = 0; i < metronomeLocalChannels.size(); ++i)
	{
		currentGraph->addConnection({ {graphID, metronomeLocalChannels[i]}, {audioOutputGraphID, metronomeOutChannels[i]} }); //can brute force, addConnection will take care of not doubling conections

		if (metronome != nullptr)
		{
			for (auto& c : metronome->ticChannelRemap)
			{
				c->setOutputChannelMapping(i, metronomeLocalChannels[i]);
			}
		}
	}
}

void AudioLayer::updatePlayConfigDetails()
{
	if (currentProcessor == nullptr) return;
	if (currentGraph == nullptr) return;

	currentProcessor->setPlayConfigDetails(numActiveInputs, numActiveOutputs, currentGraph->getSampleRate(), currentGraph->getBlockSize());
	currentProcessor->prepareToPlay(currentGraph->getSampleRate(), currentGraph->getBlockSize());
}

void AudioLayer::updateClipConfig(AudioLayerClip* clip, bool updateOutputChannelRemapping)
{
	if (clip == nullptr) return;

	clip->channelRemapAudioSource.clearAllMappings();
	//clip->channelRemapAudioSource.prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
	clip->setPlaySpeed(sequence->playSpeed->doubleValue());
	if (currentGraph != nullptr) clip->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());

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
	return volume->doubleValue();// * layer->audioModule->outVolume->doubleValue()
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

void AudioLayer::resetMetronome()
{
	if (isCurrentlyLoadingData) return;

	{
		GenericScopedLock mLock(metronomeLock);
		metronome.reset(metronomeVolume->enabled ? new Metronome(bip1File->enabled ? bip1File->getFile() : File(), bip2File->enabled ? bip2File->getFile() : File()) : nullptr);
	}

	updateSelectedOutChannels();
}


void AudioLayer::onContainerParameterChangedInternal(Parameter* p)
{
	SequenceLayer::onContainerParameterChangedInternal(p);
	if (p == metronomeVolume || p == bip1File || p == bip2File)
	{
		resetMetronome();
	}
	//else if (p == routeMonoToAll)
	//{
	//	if (!isCurrentlyLoadingData && !settingAudioGraph) updateSelectedOutChannels();
	//}
}

void AudioLayer::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	SequenceLayer::onControllableFeedbackUpdateInternal(cc, c);

	if (cc == &channelsCC || cc == &metronomeCC)
	{
		if (!isCurrentlyLoadingData && !settingAudioGraph) updateSelectedOutChannels();
	}
	else if (currentClip != nullptr)
	{
		if (c == currentClip->enabled) updateCurrentClip();
		else if (c == currentClip->time || c == currentClip->stretchFactor)
		{
			float pos = currentClip->clipStartOffset->doubleValue() + (sequence->hiResAudioTime - currentClip->time->doubleValue()) / currentClip->stretchFactor->doubleValue();
			currentClip->transportSource.setPosition(pos);

			if (c == currentClip->stretchFactor)
			{
				currentClip->setPlaySpeed(sequence->playSpeed->doubleValue());
				currentClip->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
			}
		}
	}
}

void AudioLayer::onControllableStateChanged(Controllable* c)
{
	SequenceLayer::onControllableStateChanged(c);
	if (c == metronomeVolume || c == bip1File || c == bip2File)
	{
		resetMetronome();
	}
}

void AudioLayer::selectAll(bool addToSelection)
{
	clipManager.askForSelectAllItems(addToSelection);
	setSelected(false);
}

var AudioLayer::getJSONData(bool includeNonOverriden)
{
	var data = SequenceLayer::getJSONData(includeNonOverriden);
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
}

void AudioLayer::afterLoadJSONDataInternal()
{
	resetMetronome();
	updateSelectedOutChannels();
}

void AudioLayer::fileLoaded()
{
	Engine::mainEngine->removeEngineListener(this);
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
	if (sequence->isSeeking) prevMetronomeBeat = -1;

	if (enveloppe == nullptr) return;

	if (currentProcessor != nullptr) enveloppe->setValue(currentProcessor->currentEnveloppe);
	else enveloppe->setValue(0);

	updateCurrentClip();

	if (currentClip != nullptr)
	{
		if (sequence->isSeeking)
		{
			float pos = currentClip->clipStartOffset->doubleValue() + (sequence->hiResAudioTime - currentClip->time->doubleValue()) / currentClip->stretchFactor->doubleValue();
			currentClip->transportSource.setPosition(pos);
		}

		if (currentClip->volume->controlMode == Parameter::ControlMode::AUTOMATION && currentClip->volume->automation != nullptr)
		{
			float currentTime = sequence->currentTime->doubleValue();
			float relClipStart = currentTime - currentClip->time->doubleValue();

			if (Automation* a = (Automation*)currentClip->volume->automation->automationContainer)
			{
				a->position->setValue(relClipStart);
			}
		}
	}
}

void AudioLayer::sequencePlayStateChanged(Sequence*)
{
	prevMetronomeBeat = sequence->currentTime->doubleValue() == 0 ? -2 : -1; //-2 = play from start, -1 = play from anywhere else (avoid to play sound on each "resume")

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
			float pos = currentClip->clipStartOffset->doubleValue() + (sequence->hiResAudioTime - currentClip->time->doubleValue()) / currentClip->stretchFactor->doubleValue();
			currentClip->transportSource.setPosition(pos);
			currentClip->start();
		}
	}
}

void AudioLayer::sequencePlaySpeedChanged(Sequence*)
{
	if (currentClip != nullptr)
	{
		currentClip->setPlaySpeed(sequence->playSpeed->doubleValue());
		currentClip->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
	}
}

void AudioLayer::sequencePlayDirectionChanged(Sequence*)
{
	if (currentClip != nullptr)
	{
		float pos = currentClip->clipStartOffset->doubleValue() + (sequence->hiResAudioTime - currentClip->time->doubleValue()) / currentClip->stretchFactor->doubleValue();
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

	float volumeAtStart = volume->doubleValue();
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
			|| (layer->sequence->enabled != nullptr && !layer->sequence->enabled->boolValue())
			|| !layer->sequence->isPlaying->boolValue()
			|| layer->sequence->playSpeed->doubleValue() < 0) noProcess = true;

		currentClip = layer->currentClip;

		if (!layer->metronomeVolume->enabled)
		{
			if ((currentClip.wasObjectDeleted() || currentClip.get() == nullptr)) noProcess = true;
			else if (currentClip->filePath->stringValue().isEmpty()) noProcess = true;
		}
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
		bufferToFill.buffer = &buffer;
		if (currentClip->shouldStop) currentClip->transportSource.stop();
		if ((!noProcess || currentClip->transportSource.isPlaying() || layer->clipIsStopping))
		{
			currentClip->channelRemapAudioSource.getNextAudioBlock(bufferToFill);
			if (currentClip->numChannels == 1 && layer->routeMonoToAllChannels->boolValue())
			{
				for (int i = 1; i < buffer.getNumChannels(); i++)
				{
					buffer.copyFrom(i, 0, buffer, 0, 0, buffer.getNumSamples());
				}
			}
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



	if (currentClip != nullptr)
	{

		float currentTime = layer->sequence->currentTime->doubleValue();
		float relClipStart = currentTime - currentClip->time->doubleValue();
		float relClipEnd = currentClip->getEndTime() - currentTime;

		float clipVolume = currentClip->volume->doubleValue();
		float volumeFactor = clipVolume * layer->getVolumeFactor();


		if (relClipStart < currentClip->fadeIn->doubleValue())
		{
			float fadeIn = relClipStart / currentClip->fadeIn->doubleValue();
			volumeFactor *= fadeIn * fadeIn; //square to have an ease InOut
		}

		if (relClipEnd < currentClip->fadeOut->doubleValue())
		{
			float fadeOut = relClipEnd / currentClip->fadeOut->doubleValue();
			volumeFactor *= fadeOut * fadeOut;
		}
		buffer.applyGain(volumeFactor);
	}
	else
	{
		buffer.clear();
	}


	//METRONOME
	{
		GenericScopedLock mLock(layer->metronomeLock);
		double bpm = layer->sequence->bpmPreview->enabled ? layer->sequence->bpmPreview->doubleValue() : -1;
		if (bpm > 0 && layer->metronome != nullptr && layer->sequence->playSpeed->doubleValue() > 0)
		{
			AudioSampleBuffer metronomeBuffer(buffer.getNumChannels(), buffer.getNumSamples());
			AudioSourceChannelInfo bufInfo(metronomeBuffer);

			double bpmTime = 60.0 / bpm;
			int beats = layer->sequence->beatsPerBar->intValue();
			double barTime = bpmTime * beats;
			double relBarTime = fmod(layer->sequence->hiResAudioTime, barTime) * beats / barTime; //0->beats
			int curBeat = floor(relBarTime);

			int bIndex = curBeat == 0 ? 0 : 1;
			AudioTransportSource* t = layer->metronome->ticTransports[bIndex];
			ChannelRemappingAudioSource* ch = layer->metronome->ticChannelRemap[bIndex];

			if (layer->prevMetronomeBeat != -1) //avoid launching on play after skip
			{
				if (curBeat != layer->prevMetronomeBeat)
				{
					t->setPosition(0);
					t->start();
				}

				if (layer->prevMetronomeBeat != -1)
				{
					ch->getNextAudioBlock(bufInfo);
					for (int i = 0; i < buffer.getNumChannels(); i++) buffer.addFrom(i, 0, metronomeBuffer, i, 0, buffer.getNumSamples(), layer->metronomeVolume->doubleValue());
				}
			}

			layer->prevMetronomeBeat = curBeat;
		}
	}



	if (buffer.getNumChannels() >= 2)
	{
		float panning = layer->panning->doubleValue();
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
