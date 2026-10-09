/*
  ==============================================================================

	AudioLayer.cpp
	Created: 20 Nov 2016 3:08:41pm
	Author:  Ben Kuper

  ==============================================================================
*/

#include "JuceHeader.h"

namespace { constexpr double audioLoopEndTolerance = 0.03; }

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

void AudioLayer::refreshOutputChannels()
{
	if (currentGraph == nullptr) return;

	// Rebuild the inspector channels after a device layout change, preserving
	// the selections already made for the channels that still exist.
	Array<bool> previousChannels, previousMetronome;
	for (auto* channel : channelsCC.controllables)
		previousChannels.add(static_cast<BoolParameter*>(channel)->boolValue());
	for (auto* channel : metronomeCC.controllables)
		previousMetronome.add(static_cast<BoolParameter*>(channel)->boolValue());
	settingAudioGraph = true;
	channelsCC.clear();
	metronomeCC.clear();

	const int numChannels = currentGraph->getMainBusNumOutputChannels();
	const AudioChannelSet channelSet = currentGraph->getChannelLayoutOfBus(false, 0);
	for (int i = 0; i < numChannels; ++i)
	{
		const String name = "Channel " + String(i + 1) + " : " + AudioChannelSet::getChannelTypeName(channelSet.getTypeOfChannel(i));
		channelsCC.addBoolParameter(name, "If enabled, sends audio from this layer to this channel",
			i < previousChannels.size() ? previousChannels[i] : i < 2);
		metronomeCC.addBoolParameter(name, "If enabled, sends the metronome to this channel",
			i < previousMetronome.size() ? previousMetronome[i] : i < 2);
	}
	settingAudioGraph = false;
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
    const ScopedLock lock(clipManager.items.getLock());
    AudioLayerClip* first = nullptr;
    const double time = sequence->currentTime->doubleValue();
    for (auto* item : clipManager.items)
    {
        auto* clip = static_cast<AudioLayerClip*>(item);
        const bool atLoopEnd = sequence->isPlaying->boolValue() && sequence->loopParam->boolValue()
            && clip->time->doubleValue() <= .0001
            && clip->getEndTime() + audioLoopEndTolerance >= sequence->totalTime->doubleValue()
            && time >= clip->getEndTime() && time <= sequence->totalTime->doubleValue();
        const bool active = enabled->boolValue() && clip->enabled->boolValue() && (clip->isInRange(time) || atLoopEnd);
        if (active && first == nullptr) first = clip;
        if (clip->isActive->boolValue() == active) continue;
        clip->isActive->setValue(active);
        if (active)
        {
            clip->transportSource.setPosition(clip->clipStartOffset->doubleValue()
                + (sequence->hiResAudioTime - clip->time->doubleValue()) / clip->stretchFactor->doubleValue());
            if (sequence->isPlaying->boolValue()) clip->start();
        }
    }
    if (first != currentClip)
    {
        currentClip = first;
        requestAudioDeclick();
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
	if (!clip->resizeSequenceOnLoad)
	{
		clip->resizeSequenceOnLoad = true;
		return;
	}
	if (clip->clipDuration <= 0) return;

	// Keep the end of existing content, including other clips, keys and cues.
	// Ignore the importing clip's previous length when its file is replaced.
	double contentEnd = sequence->minSequenceTime;
	for (auto* layer : sequence->layerManager->items)
	{
		if (layer == this)
		{
			for (auto* other : clipManager.items)
				if (other != clip) contentEnd = jmax(contentEnd, (double) other->getEndTime());
		}
		else
		{
			Array<float> times;
			layer->getSnapTimes(&times);
			for (float time : times) contentEnd = jmax(contentEnd, (double) time);

			// Video layers do not expose their clip edges as snap times.
			if (auto* video = dynamic_cast<VideoLayer*>(layer))
				for (auto* videoClip : video->clipManager.items)
					contentEnd = jmax(contentEnd, (double) videoClip->getEndTime());
		}
	}

	Array<float> cueTimes;
	sequence->cueManager->getSnapTimes(&cueTimes);
	for (float time : cueTimes) contentEnd = jmax(contentEnd, (double) time);

	const double length = jmax(contentEnd, (double) clip->getEndTime());
	if (std::abs(length - sequence->totalTime->doubleValue()) > 0.0001)
		sequence->totalTime->setUndoableValue(sequence->totalTime->doubleValue(), length);
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
    if (p == enabled) updateCurrentClip();
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
    else if (auto* clip = dynamic_cast<AudioLayerClip*>(c->parentContainer.get()))
    {
        if (c == clip->enabled || c == clip->time || c == clip->coreLength) updateCurrentClip();
        if (clip->isActive->boolValue() && (c == clip->time || c == clip->stretchFactor || c == clip->clipStartOffset))
        {
            clip->transportSource.setPosition(clip->clipStartOffset->doubleValue()
                + (sequence->hiResAudioTime - clip->time->doubleValue()) / clip->stretchFactor->doubleValue());
            if (c == clip->stretchFactor && currentGraph != nullptr)
            {
                clip->setPlaySpeed(sequence->playSpeed->doubleValue());
                clip->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
            }
            requestAudioDeclick();
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

void AudioLayer::sequenceLooped(Sequence*)
{
	sequenceLoopPending.store(true, std::memory_order_relaxed);
}

void AudioLayer::sequenceCurrentTimeChanged(Sequence*, float, bool)
{
	const bool loopSeek = sequenceLoopPending.exchange(false, std::memory_order_relaxed);
	if (sequence->isSeeking) prevMetronomeBeat = -1;

	if (enveloppe == nullptr) return;

	if (currentProcessor != nullptr) enveloppe->setValue(currentProcessor->currentEnveloppe);
	else enveloppe->setValue(0);

	updateCurrentClip();

    const ScopedLock lock(clipManager.items.getLock());
    for (auto* item : clipManager.items)
    {
        auto* clip = static_cast<AudioLayerClip*>(item);
        if (!clip->isActive->boolValue()) continue;
        if (sequence->isSeeking)
        {
            const double pos = clip->clipStartOffset->doubleValue() + (sequence->hiResAudioTime - clip->time->doubleValue()) / clip->stretchFactor->doubleValue();
            const double tolerance = currentGraph != nullptr && currentGraph->getSampleRate() > 0
                ? 2.0 * currentGraph->getBlockSize() / currentGraph->getSampleRate() + .005 : 0;
            const bool alreadyWrapped = loopSeek && clip->transportSource.isPlaying()
                && std::abs(clip->transportSource.getCurrentPosition() - pos) <= tolerance;
            if (!alreadyWrapped)
            {
                clip->transportSource.setPosition(pos);
                if (sequence->isPlaying->boolValue() && enabled->boolValue()) clip->start();
                requestAudioDeclick();
            }
        }
        if (clip->volume->controlMode == Parameter::AUTOMATION && clip->volume->automation)
            if (auto* a = dynamic_cast<Automation*>(clip->volume->automation->automationContainer))
                a->position->setValue(sequence->currentTime->doubleValue() - clip->time->doubleValue());
    }
}

void AudioLayer::sequencePlayStateChanged(Sequence*)
{
    updateCurrentClip();
    prevMetronomeBeat = sequence->currentTime->doubleValue() == 0 ? -2 : -1;
    if (!sequence->isPlaying->boolValue()) enveloppe->setValue(0);
    const ScopedLock lock(clipManager.items.getLock());
    for (auto* item : clipManager.items)
    {
        auto* clip = static_cast<AudioLayerClip*>(item);
        if (!sequence->isPlaying->boolValue() || !clip->isActive->boolValue()) clip->stop();
        else
        {
            clip->transportSource.setPosition(clip->clipStartOffset->doubleValue()
                + (sequence->hiResAudioTime - clip->time->doubleValue()) / clip->stretchFactor->doubleValue());
            clip->start();
        }
    }
    requestAudioDeclick();
}

void AudioLayer::sequencePlaySpeedChanged(Sequence*)
{
    const ScopedLock lock(clipManager.items.getLock());
    for (auto* item : clipManager.items)
    {
        auto* clip = static_cast<AudioLayerClip*>(item);
        clip->setPlaySpeed(sequence->playSpeed->doubleValue());
        if (currentGraph) clip->prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
    }
}

void AudioLayer::sequencePlayDirectionChanged(Sequence*)
{
    const ScopedLock lock(clipManager.items.getLock());
    for (auto* item : clipManager.items)
    {
        auto* clip = static_cast<AudioLayerClip*>(item);
        if (clip->isActive->boolValue())
            clip->transportSource.setPosition(clip->clipStartOffset->doubleValue()
                + (sequence->hiResAudioTime - clip->time->doubleValue()) / clip->stretchFactor->doubleValue());
    }
    requestAudioDeclick();
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
	declickSamples = jmax(1, roundToInt(sampleRate * 0.005));
	declickSamplesRemaining = 0;
	clipBuffer.setSize(jmax(1, getTotalNumOutputChannels()), maximumExpectedSamplesPerBlock);
	lastAudioDiscontinuity = layer != nullptr ? layer->audioDiscontinuityCounter.load(std::memory_order_relaxed) : 0;
	lastOutputSamples.assign((size_t) jmax(0, getTotalNumOutputChannels()), 0.0f);
	transitionStartSamples.assign(lastOutputSamples.size(), 0.0f);
}

void AudioLayerProcessor::releaseResources()
{
	declickSamplesRemaining = 0;
	std::fill(lastOutputSamples.begin(), lastOutputSamples.end(), 0.0f);
}

void AudioLayerProcessor::applyDeclick(AudioBuffer<float>& buffer)
{
	const unsigned int discontinuity = layer->audioDiscontinuityCounter.load(std::memory_order_relaxed);
	if (discontinuity != lastAudioDiscontinuity)
	{
		lastAudioDiscontinuity = discontinuity;
		std::copy(lastOutputSamples.begin(), lastOutputSamples.end(), transitionStartSamples.begin());
		declickSamplesRemaining = declickSamples;
	}

	const int channels = jmin(buffer.getNumChannels(), (int) lastOutputSamples.size());
	for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
	{
		if (declickSamplesRemaining > 0)
		{
			const float newGain = (float) (declickSamples - declickSamplesRemaining) / (float) declickSamples;
			for (int channel = 0; channel < channels; ++channel)
			{
				const float value = buffer.getSample(channel, sample);
				buffer.setSample(channel, sample, transitionStartSamples[(size_t) channel] * (1.0f - newGain) + value * newGain);
			}
			--declickSamplesRemaining;
		}

		for (int channel = 0; channel < channels; ++channel)
			lastOutputSamples[(size_t) channel] = buffer.getSample(channel, sample);
	}
}

void AudioLayerProcessor::applyClipEdgeFade(AudioBuffer<float>& buffer, AudioLayerClip& clip, double sourcePosition, int startSample, int numSamples)
{
	if (getSampleRate() <= 0 || clip.clipDuration <= 0 || clip.stretchFactor->doubleValue() <= 0) return;

	const double clipEnd = jmin((double) clip.getEndTime(), layer->sequence->totalTime->doubleValue());
	const double sourceStart = clip.clipStartOffset->doubleValue();
	const double sourceEnd = jmin(clip.clipDuration,
		sourceStart + (clipEnd - clip.time->doubleValue()) / clip.stretchFactor->doubleValue());
	const double secondsPerSample = 1.0 / getSampleRate();

	for (int sample = startSample; sample < startSample + numSamples; ++sample)
	{
		const double position = sourcePosition + (sample - startSample) * secondsPerSample;
		const float gain = (float) jlimit(0.0, 1.0,
			jmin(position - sourceStart, sourceEnd - position) / 0.005);
		if (gain < 1.0f) buffer.applyGain(sample, 1, gain);
	}
}

void AudioLayerProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    const bool noProcess = layer == nullptr || !layer->enabled->boolValue()
        || (layer->sequence->enabled && !layer->sequence->enabled->boolValue())
        || !layer->sequence->isPlaying->boolValue() || layer->sequence->playSpeed->doubleValue() < 0
        || buffer.getNumChannels() == 0 || layer->currentGraph == nullptr
        || layer->currentGraph->getBlockSize() <= 0 || layer->currentGraph->getSampleRate() <= 0;
    buffer.clear();
    if (noProcess)
    {
        currentEnveloppe = 0; rmsCount = 0; tempRMS = 0;
        if (layer) applyDeclick(buffer);
        return;
    }
    AudioSourceChannelInfo bufferToFill(buffer);
    {
        const ScopedLock lock(layer->clipManager.items.getLock());
        clipBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
        for (auto* item : layer->clipManager.items)
        {
            auto* clip = static_cast<AudioLayerClip*>(item);
            if (!clip->enabled->boolValue() || !clip->isActive->boolValue() || clip->isLoading
                || clip->filePath->stringValue().isEmpty()) continue;
            clipBuffer.clear();
            AudioSourceChannelInfo clipInfo(clipBuffer);
            double clipSourcePosition = 0, clipSourcePositionAfterWrap = 0;
            int clipSplitSample = clipInfo.numSamples;
			clipSourcePosition = clip->transportSource.getCurrentPosition();
			const double sequenceEnd = layer->sequence->totalTime->doubleValue();
			if (!noProcess && layer->sequence->loopParam->boolValue()
				&& clip->time->doubleValue() <= 0.0001
				// Compressed files can report a duration a few milliseconds short of the timeline.
				&& clip->getEndTime() + audioLoopEndTolerance >= sequenceEnd
				&& clip->stretchFactor->doubleValue() > 0
				&& clip->clipDuration > 0)
			{
			const double sourceStart = clip->clipStartOffset->doubleValue();
			const double sourceEnd = jmin(clip->clipDuration,
				sourceStart + sequenceEnd / clip->stretchFactor->doubleValue());
			const double samplesUntilWrap = (sourceEnd - clipSourcePosition) * getSampleRate();
			if (sourceEnd > sourceStart && getSampleRate() > 0
				&& samplesUntilWrap <= clipInfo.numSamples)
			{
				clipSplitSample = jlimit(0, clipInfo.numSamples, (int) std::ceil(samplesUntilWrap));
			}
			}

			if (clipSplitSample > 0)
			{
				AudioSourceChannelInfo firstPart(clipInfo);
				firstPart.numSamples = clipSplitSample;
				clip->channelRemapAudioSource.getNextAudioBlock(firstPart);
			}
			if (clipSplitSample < clipInfo.numSamples)
			{
				const double sourceStart = clip->clipStartOffset->doubleValue();
				clip->transportSource.setPosition(sourceStart);
				clip->start();
				clipSourcePositionAfterWrap = clip->transportSource.getCurrentPosition();
				AudioSourceChannelInfo secondPart(clipInfo);
				secondPart.startSample += clipSplitSample;
				secondPart.numSamples -= clipSplitSample;
				clip->channelRemapAudioSource.getNextAudioBlock(secondPart);
			}
            const auto fades = clip->getEffectiveFades();
            auto* automation = clip->volume->controlMode == Parameter::AUTOMATION && clip->volume->automation
                ? dynamic_cast<Automation*>(clip->volume->automation->automationContainer) : nullptr;
            const double stretch = clip->stretchFactor->doubleValue();
            const double layerGain = layer->getVolumeFactor(), staticVolume = clip->volume->doubleValue();
            const double offset = clip->clipStartOffset->doubleValue(), length = clip->coreLength->doubleValue();
            const ScopedLock curveLock(automation ? automation->items.getLock() : layer->clipManager.items.getLock());
            for (int sample = 0; sample < clipInfo.numSamples; ++sample)
            {
                const bool wrapped = sample >= clipSplitSample;
                const double sourceTime = (wrapped ? clipSourcePositionAfterWrap : clipSourcePosition)
                    + (sample - (wrapped ? clipSplitSample : 0)) / getSampleRate();
                const double localTime = (sourceTime - offset) * stretch;
                const double volume = automation ? automation->getValueAtPosition(localTime) : staticVolume;
                const float gain = (float)(volume * layerGain
                    * BlockTransitions::gain(localTime, length, fades));
                clipBuffer.applyGain(sample, 1, gain);
            }
            applyClipEdgeFade(clipBuffer, *clip, clipSourcePosition, 0, clipSplitSample);
            if (clipSplitSample < clipInfo.numSamples)
                applyClipEdgeFade(clipBuffer, *clip, clipSourcePositionAfterWrap, clipSplitSample, clipInfo.numSamples - clipSplitSample);
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.addFrom(channel, 0, clipBuffer,
                    clip->numChannels == 1 && layer->routeMonoToAllChannels->boolValue() ? 0 : channel,
                    0, buffer.getNumSamples());
        }
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
	applyDeclick(buffer);

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
