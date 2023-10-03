/*
  ==============================================================================

    SequenceManager.h
    Created: 28 Oct 2016 8:13:04pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceLayer;
class TimeCue;
class AudioLayer;
class TimeTrigger;

class SequenceManager :
	public BaseManager<Sequence>,
	public Sequence::SequenceListener
{
public:
#if TIMELINE_USE_SEQUENCEMANAGER_SINGLETON
	juce_DeclareSingleton(SequenceManager, false)
#endif

	SequenceManager();
	~SequenceManager();

	Trigger* playAllTrigger;
	Trigger* stopAllTrigger;
	BoolParameter* onlyOneSequencePlaying;
	BoolParameter* isOneSequencePlaying;

	Factory<SequenceLayer>* defaultLayerFactory;

	virtual void addItemInternal(Sequence* item, var data) override;
	virtual void removeItemInternal(Sequence* s) override;

	virtual void onContainerTriggerTriggered(Trigger*) override;

	virtual void sequencePlayStateChanged(Sequence* s) override;

	void showMenuAndGetSequence(ControllableContainer* startFromCC, std::function<void(Sequence *)> returnFunc);
	Sequence * getSequenceForItemID(int itemID);

	void showMenuAndGetLayer(ControllableContainer* startFromCC, std::function<void(SequenceLayer*)> returnFunc);
	SequenceLayer * getLayerForItemID(int itemID);

	void showMenuAndGetCue(ControllableContainer* startFromCC, std::function<void(TimeCue*)> returnFunc);
	TimeCue * getCueForItemID(int itemID);


	void showMenuAndGetAudioLayer(ControllableContainer* startFromCC, std::function<void(AudioLayer*)> returnFunc);
	AudioLayer* getAudioLayerForItemID(int itemID);

	void showMenuAndGetTrigger(ControllableContainer* startFromCC, std::function<void(TimeTrigger*)> returnFunc);

	std::unique_ptr<FileChooser> fileChooser;
	virtual void importMultipleAudioFiles();
	virtual void createSequenceFromAudioFile(File f);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceManager)
};