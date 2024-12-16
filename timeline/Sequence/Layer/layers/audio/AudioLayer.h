/*
  ==============================================================================

	AudioLayer.h
	Created: 20 Nov 2016 3:08:41pm
	Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class AudioLayerProcessor;

class AudioLayer :
	public SequenceLayer,
	public AudioLayerClip::ClipListener,
	public AudioLayerClipManager::ManagerListener,
	public Thread,
	public Inspectable::InspectableListener
{
public:
	AudioLayer(Sequence* sequence, var params);
	~AudioLayer();

	AudioLayerClipManager clipManager;

	AudioProcessorGraph* currentGraph;

	WeakReference<AudioLayerClip> currentClip;
	AudioLayerProcessor* currentProcessor;

	ControllableContainer channelsCC;
	Array<int> selectedOutChannels;
	Array<int> clipLocalChannels;
	var channelsData; //for ghosting

	FloatParameter* volume;
	FloatParameter* panning;
	FloatParameter* enveloppe;

	int numActiveInputs;
	int numActiveOutputs;

	AudioProcessorGraph::NodeID graphID;
	static int graphIDIncrement;
	AudioProcessorGraph::NodeID audioOutputGraphID;

	//Volume animation
	float targetVolume;
	Automation* volumeInterpolationAutomation;
	WeakReference<Inspectable> volumeAutomationRef;
	float volumeInterpolationTime;
	bool stopAtVolumeInterpolationFinish;

	//thread transportSource stop flag
	bool clipIsStopping;

	FloatParameter* metronomeVolume;
	FileParameter* bip1File;
	FileParameter* bip2File;

	ControllableContainer metronomeCC;
	Array<int> metronomeOutChannels;
	Array<int> metronomeLocalChannels;
	std::unique_ptr<Metronome> metronome;
	SpinLock metronomeLock;
	int prevMetronomeBeat;
	var metronomeData; //for ghosting

	//safety
	bool settingAudioGraph;

	virtual void clearItem() override;

	void setAudioProcessorGraph(AudioProcessorGraph* graph, AudioProcessorGraph::NodeID graphOutputID = AudioProcessorGraph::NodeID(2));
	virtual AudioLayerProcessor* createAudioLayerProcessor();

	virtual int getNodeGraphIDIncrement() { return graphIDIncrement++; }

	virtual AudioLayerClip* createAudioClip();

	virtual void updateCurrentClip();

	void itemAdded(LayerBlock* clip) override;
	void itemsAdded(Array<LayerBlock*> clips) override;
	void itemRemoved(LayerBlock* clip) override;
	void itemsRemoved(Array<LayerBlock*> clips) override;

	void clipSourceLoaded(AudioLayerClip* clip) override;

	virtual void updateSelectedOutChannels();
	virtual void updateSelectedOutChannelsInternal() {}
	void updatePlayConfigDetails();

	void updateClipConfig(AudioLayerClip* clip, bool updateOutputChannelRemapping = true);

	virtual float getVolumeFactor();
	virtual void setVolume(float value, float time = 0, Automation* automation = nullptr, bool stopSequenceAtFinish = false);

	void resetMetronome();

	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
	void onControllableStateChanged(Controllable* c) override;

	void selectAll(bool addToSelection = false) override;

	virtual var getJSONData(bool includeNonOverriden = false) override;
	virtual void loadJSONDataInternal(var data) override;

	virtual void afterLoadJSONDataInternal() override;

	virtual SequenceLayerPanel* getPanel() override;
	virtual SequenceLayerTimeline* getTimelineUI() override;

	void sequenceCurrentTimeChanged(Sequence*, float prevTime, bool evaluatedSkippedData) override;
	void sequencePlayStateChanged(Sequence*) override;
	void sequencePlaySpeedChanged(Sequence*) override;
	void sequencePlayDirectionChanged(Sequence*) override;

	static AudioLayer* create(Sequence* sequence, var params) { return new AudioLayer(sequence, params); }
	virtual String getTypeString() const override { return "Audio"; }

	virtual void getSnapTimes(Array<float>* arrayToFill) override;

	//For volume interpolation
	void run() override;

	void inspectableDestroyed(Inspectable* i) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLayer)

};


class AudioLayerProcessor :
	public AudioProcessor
{
public:
	AudioLayerProcessor(AudioLayer* layer);
	~AudioLayerProcessor();

	AudioLayer* layer;

	const int minEnveloppeSamples = 1024;
	int rmsCount;
	float tempRMS;
	float currentEnveloppe;

	void clear();

	// Hérité via AudioProcessor
	virtual const String getName() const override;
	virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	virtual void releaseResources() override;
	virtual void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	virtual double getTailLengthSeconds() const override;
	virtual bool acceptsMidi() const override;
	virtual bool producesMidi() const override;
	virtual AudioProcessorEditor* createEditor() override;
	virtual bool hasEditor() const override;
	virtual int getNumPrograms() override;
	virtual int getCurrentProgram() override;
	virtual void setCurrentProgram(int index) override;
	virtual const String getProgramName(int index) override;
	virtual void changeProgramName(int index, const String& newName) override;
	virtual void getStateInformation(juce::MemoryBlock& destData) override;
	virtual void setStateInformation(const void* data, int sizeInBytes) override;
};
