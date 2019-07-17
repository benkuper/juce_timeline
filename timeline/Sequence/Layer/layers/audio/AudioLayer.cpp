#include "AudioLayer.h"
/*
  ==============================================================================

	AudioLayer.cpp
	Created: 20 Nov 2016 3:08:41pm
	Author:  Ben Kuper

  ==============================================================================
*/

int AudioLayer::graphIDIncrement = 10;

AudioLayer::AudioLayer(Sequence * _sequence, var params) :
	SequenceLayer(_sequence, "Audio"),
	enveloppe(nullptr),
	currentGraph(nullptr),
	currentProcessor(nullptr),
	channelsCC("Channels"),
	numActiveOutputs(0), 
    graphID(0) //was -1 but since 5.2.1, generated warning. Should do otherwise ?
{

	helpID = "AudioLayer";

	volume = addFloatParameter("Volume", "Volume multiplier for the layer", 1, 0, 10);
	panning = addFloatParameter("Panning","Left/Right simple panning",0,-1,1);
    
	enveloppe = addFloatParameter("Enveloppe", "Enveloppe", 0, 0, 1);
	enveloppe->isControllableFeedbackOnly = true;

	addChildControllableContainer(&channelsCC);
	addChildControllableContainer(&clipManager);

	clipManager.addBaseManagerListener(this);
	helpID = "AudioLayer";

}

AudioLayer::~AudioLayer()
{
	clearItem();
}

void AudioLayer::clearItem()
{
	setAudioProcessorGraph(nullptr);
	clipManager.clear();
	SequenceLayer::clearItem();
}

void AudioLayer::setAudioProcessorGraph(AudioProcessorGraph * graph, int outputGraphID)
{
	if (currentGraph != nullptr)
	{
		currentGraph->removeNode(graphID);
		currentProcessor->clear();
		currentProcessor = nullptr;

		if (!isCurrentlyLoadingData)
		{
			channelsData = channelsCC.getJSONData();
			DBG("KEEP ghost " << channelsData.toString());
		}
		
		channelsCC.clear();
	}

	currentGraph = graph;

	if (currentGraph != nullptr)
	{
        
		auto proc = std::unique_ptr<AudioLayerProcessor>(new AudioLayerProcessor(this));
		currentProcessor = proc.get(); 
		
		graphID = AudioProcessorGraph::NodeID(AudioLayer::graphIDIncrement++);
		currentGraph->addNode(std::move(proc), graphID);
		
		int numChannels = currentGraph->getMainBusNumOutputChannels();
		AudioChannelSet channelSet = currentGraph->getChannelLayoutOfBus(false, 0);
		for (int i = 0; i < numChannels; i++)
		{
			String channelName = AudioChannelSet::getChannelTypeName(channelSet.getTypeOfChannel(i));
			BoolParameter * b = channelsCC.addBoolParameter("Channel Out : " + channelName, "If enabled, sends audio from this layer to this channel", false);
			b->setValue(i < 2, false);
		}
	}

	audioOutputGraphID = outputGraphID;

	updateSelectedOutChannels();
}


void AudioLayer::updateCurrentClip()
{
	AudioLayerClip * target = nullptr;
	
	if (sequence->currentTime->floatValue() > 0 || sequence->isPlaying->boolValue()) // only find a clip if sequence not at 0 or is playing
	{
		if (!currentClip.wasObjectDeleted() && currentClip != nullptr && currentClip->isInRange(sequence->currentTime->floatValue())) return;
		target = clipManager.getClipAtTime(sequence->currentTime->floatValue());
	}

	if (target == currentClip) return;

	if (currentClip != nullptr)
	{
		currentClip->setIsCurrent(false);

	}

	currentClip = target;
	
	if (currentClip != nullptr)
	{
		currentClip->setIsCurrent(true);
		if (sequence->currentManager != nullptr) currentClip->transportSource.prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
		currentClip->transportSource.setPosition(sequence->hiResAudioTime - currentClip->time->floatValue()); 
		if (sequence->isPlaying->boolValue()) currentClip->transportSource.start();
	}

}

void AudioLayer::itemAdded(AudioLayerClip*)
{
	if (Engine::mainEngine->isLoadingFile) return;
	//updateCurrentClip();
}

void AudioLayer::itemRemoved(AudioLayerClip* clip)
{
	if (clip == currentClip) updateCurrentClip();
}

void AudioLayer::updateSelectedOutChannels()
{
	
	selectedOutChannels.clear();

	if (currentGraph == nullptr) return;

	currentGraph->disconnectNode(graphID);

	numActiveOutputs = 0;
	for (int i = 0; i < channelsCC.controllables.size(); i++) if (((BoolParameter *)channelsCC.controllables[i])->boolValue()) numActiveOutputs++;


	currentProcessor->setPlayConfigDetails(0, numActiveOutputs, currentGraph->getSampleRate(), currentGraph->getBlockSize());
	currentProcessor->prepareToPlay(currentGraph->getSampleRate(), currentGraph->getBlockSize());

	
	for (auto & c : clipManager.items)
	{
		c->channelRemapAudioSource.clearAllMappings();
		c->channelRemapAudioSource.prepareToPlay(currentGraph->getBlockSize(), currentGraph->getSampleRate());
	}

	int index = 0;
	for (int i = 0; i < channelsCC.controllables.size(); i++)
	{
		if (((BoolParameter *)channelsCC.controllables[i])->boolValue())
		{
			selectedOutChannels.add(i);
			currentGraph->addConnection({{AudioProcessorGraph::NodeID(graphID), index }, {(AudioProcessorGraph::NodeID)audioOutputGraphID, i } });
			for (auto & c : clipManager.items) c->channelRemapAudioSource.setOutputChannelMapping(index, index);
			index++;
		}
	}

	numActiveOutputs = selectedOutChannels.size();

}

float AudioLayer::getVolumeFactor()
{
	return volume->floatValue();// * layer->audioModule->outVolume->floatValue()
}

void AudioLayer::onControllableFeedbackUpdateInternal(ControllableContainer * cc, Controllable * c)
{
	SequenceLayer::onControllableFeedbackUpdateInternal(cc, c);

	if (cc == &channelsCC)
	{
		updateSelectedOutChannels();
	}
	else if (currentClip != nullptr && c == currentClip->enabled)
	{
		updateCurrentClip();
	}
}

var AudioLayer::getJSONData()
{
	var data = SequenceLayer::getJSONData();
	data.getDynamicObject()->setProperty("clipManager", clipManager.getJSONData());
	if (currentGraph != nullptr)
	{
		data.getDynamicObject()->setProperty("channels", channelsCC.getJSONData());
	}

	return data;
}

void AudioLayer::loadJSONDataInternal(var data)
{
	channelsData = data.getProperty("channels", var());

	SequenceLayer::loadJSONDataInternal(data);
	clipManager.loadJSONData(data.getProperty("clipManager", var()));
}

SequenceLayerPanel * AudioLayer::getPanel()
{
	return new AudioLayerPanel(this);
}

SequenceLayerTimeline * AudioLayer::getTimelineUI()
{
	return new AudioLayerTimeline(this);
}

void AudioLayer::sequenceCurrentTimeChanged(Sequence *, float, bool)
{
	if (enveloppe == nullptr) return;

	if (currentProcessor != nullptr) enveloppe->setValue(currentProcessor->currentEnveloppe);
	else enveloppe->setValue(0);

	updateCurrentClip();

	if (currentClip != nullptr && sequence->isSeeking)
	{
		currentClip->transportSource.setPosition(sequence->hiResAudioTime - currentClip->time->floatValue());
	}
}

void AudioLayer::sequencePlayStateChanged(Sequence *)
{
	if (!sequence->isPlaying->boolValue())
	{
		enveloppe->setValue(0);
		if (currentClip != nullptr) currentClip->transportSource.stop();
	}
	else
	{

		if (currentClip != nullptr)
		{
			currentClip->transportSource.setPosition(sequence->hiResAudioTime - currentClip->time->floatValue()); 
			currentClip->transportSource.start();
		}
	}
}


//Audio Processor

AudioLayerProcessor::AudioLayerProcessor(AudioLayer * _layer) :
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
	return String();
}

void AudioLayerProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
}

void AudioLayerProcessor::releaseResources()
{

}

void AudioLayerProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer & midiMessages)
{
	bool noProcess = false;

	WeakReference<AudioLayerClip> currentClip;

	if (layer != nullptr)
	{
		if (!layer->enabled->boolValue()) noProcess = true;;
		currentClip = layer->currentClip;
		if (currentClip.wasObjectDeleted() || currentClip.get() == nullptr) noProcess = true;
		else if (currentClip->filePath->stringValue().isEmpty()) noProcess = true;
	} else
	{
		noProcess = true;
	}

	if (buffer.getNumChannels() == 0)
	{
		noProcess = true;
	}

	if (noProcess)
	{
		currentEnveloppe = 0;
		rmsCount = 0;
		tempRMS = 0;
        buffer.clear();
        return;
	}
	
	float volumeFactor = currentClip->volume->floatValue() * layer->getVolumeFactor();
	layer->currentClip->transportSource.setGain(volumeFactor);

	AudioSourceChannelInfo bufferToFill;
	bufferToFill.buffer = &buffer;
	bufferToFill.startSample = 0;
	bufferToFill.numSamples = buffer.getNumSamples();

	layer->currentClip->channelRemapAudioSource.getNextAudioBlock(bufferToFill);
	
	if (buffer.getNumChannels() >= 2)
	{
		float panning = layer->panning->floatValue();
		if (panning < 0) buffer.applyGain(1, bufferToFill.startSample, bufferToFill.numSamples, 1 + panning);
		else if (panning > 0) buffer.applyGain(0, bufferToFill.startSample, bufferToFill.numSamples, 1 - panning);
	}
	
	float rms = 0;
	for (int i = 0; i < buffer.getNumChannels(); i++)
	{
		rms = jmax(rms, buffer.getRMSLevel(i, bufferToFill.startSample, bufferToFill.numSamples));
	}

	layer->enveloppe->setValue(rms);

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

AudioProcessorEditor * AudioLayerProcessor::createEditor()
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

void AudioLayerProcessor::changeProgramName(int index, const String & newName)
{
}

void AudioLayerProcessor::getStateInformation(juce::MemoryBlock & destData)
{
}

void AudioLayerProcessor::setStateInformation(const void * data, int sizeInBytes)
{
}
