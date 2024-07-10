/*
  ==============================================================================

    Sequence.h
    Created: 28 Oct 2016 8:13:19pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceLayerManager;
class TimeCueManager;
class TimeCue;

class Sequence :
	public BaseItem,
	public Thread,
	public AudioIODeviceCallback,
	public EngineListener
{
public:
	Sequence();
	virtual ~Sequence();

	std::unique_ptr<TimeCueManager> cueManager;
	std::unique_ptr<SequenceLayerManager> layerManager;

	BoolParameter * startAtLoad;
	FloatParameter * totalTime;
	FloatParameter * currentTime;
	BoolParameter* includeCurrentTimeInSave;
	FloatParameter * playSpeed;
	BoolParameter * loopParam;
	IntParameter * fps;
	BoolParameter* autoSnap;

	enum EvaluateMode { NEVER, ONLY_PLAYING, ONLY_NOT_PLAYING, ALWAYS };
	EnumParameter* evaluateOnSeek;

	FloatParameter* bpmPreview;
	IntParameter * beatsPerBar;


	Trigger * playTrigger;
	Trigger * pauseTrigger;
	Trigger * stopTrigger;
	Trigger * finishTrigger;
	Trigger * togglePlayTrigger;
	Trigger * prevCue;
	Trigger*  nextCue;

	BoolParameter * isPlaying;

    AudioDeviceManager * currentManager;
    
	double hiResAudioTime;

	double sampleRate;

	bool isSeeking;

	//Playback
	double prevTime;
	//double timeAtSetTime;
	double millisAtSetTime;
	double prevMillis;
	double targetTime;
	float prevSpeed;

	//UI
	const float minSequenceTime = 1; //in seconds
	FloatParameter * viewStartTime;
	FloatParameter * viewEndTime;
	FloatParameter* minViewTime;
	BoolParameter* viewFollowTime;
	float followViewRange;
	bool isBeingEdited;

	CriticalSection sequenceTimeLock;

	virtual void clearItem() override;

	void setCurrentTime(float time, bool forceOverPlaying = true, bool seekMode = false);

	void handleCueAction(TimeCue* cue, TimeCue* originCue = nullptr);

	int getFrameForTime(float time, bool forceDirection = false, bool forcePrev = true);
	double getTimeForFrame(float fame);
	double getNextFrameTimeForTime(float time);
	double getPrevFrameTimeForTime(float time);

	void setBeingEdited(bool value);

	void selectAllItemsBetween(float start, float end);
	void removeAllItemsBetween(float start, float end);
	void removeTimespan(float start, float end);
	void insertTimespan(float start, float length);

	virtual void getSnapTimes(Array<float>* arrayToFill, float start = 0, float end = -1, const Array<float> &excludeValues = Array<float>());
	virtual float getClosestSnapTimeFor(Array<float> snapTimes, float t);

	virtual bool paste() override;

	void setAudioDeviceManager(AudioDeviceManager * am);
	virtual void updateSampleRate();

	// Inherited via AudioIODeviceCallback
	virtual void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
		int numInputChannels,
		float* const* outputChannelData,
		int numOutputChannels,
		int numSamples,
		const AudioIODeviceCallbackContext& context) override;

	virtual void audioDeviceAboutToStart(AudioIODevice * device) override;
	virtual void audioDeviceStopped() override;

	virtual bool timeIsDrivenByAudio();
	
	virtual void onContainerParameterChangedInternal(Parameter*) override;
	virtual void onContainerTriggerTriggered(Trigger*) override;
	virtual void onControllableStateChanged(Controllable* c) override;

	virtual void parameterControlModeChanged(Parameter* p) override;

	virtual bool handleRemoteControlData(Controllable* c, const juce::OSCMessage& m, const juce::String& cliendId) override;

	virtual String getPanelName() const;
	
	virtual void run() override;

	virtual void endLoadFile() override;
	virtual void handleStartAtLoad();

	virtual var getJSONData() override;
	virtual void loadJSONDataInternal(var data) override;

	DECLARE_TYPE("Sequence");

	class SequenceListener
	{
	public:
		virtual ~SequenceListener() {}
		virtual void sequencePlayStateChanged(Sequence *) {}
		virtual void sequenceCurrentTimeChanged(Sequence *, float /*prevTime*/, bool /*evaluateSkippedData*/) {}
		virtual void sequenceFinished(Sequence*) {}
		virtual void sequenceLooped(Sequence *) {}
		virtual void sequencePlaySpeedChanged(Sequence*) {}
		virtual void sequencePlayDirectionChanged(Sequence*) {}
		virtual void sequenceTotalTimeChanged(Sequence *) {}
		virtual void sequenceMasterAudioModuleChanged(Sequence *) {}
		virtual void sequenceEditingStateChanged(Sequence *) {}
	};

	DECLARE_INSPECTACLE_CRITICAL_LISTENER(Sequence, sequence);
	DECLARE_ASYNC_EVENT(Sequence, Sequence, sequence, ENUM_LIST(EDITING_STATE_CHANGED, PLAY_STATE_CHANGED), EVENT_ITEM_CHECK);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sequence)

};