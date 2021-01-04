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

	Factory<SequenceLayer>* defaultLayerFactory;

	virtual void addItemInternal(Sequence* item, var data) override;
	virtual void removeItemInternal(Sequence* s) override;

	virtual void onContainerTriggerTriggered(Trigger*) override;

	virtual void sequencePlayStateChanged(Sequence* s) override;

	Sequence * showMenuAndGetSequence();
	Sequence * getSequenceForItemID(int itemID);

	SequenceLayer * showmMenuAndGetLayer();
	SequenceLayer * getLayerForItemID(int itemID);

	TimeCue * showMenuAndGetCue();
	TimeCue * getCueForItemID(int itemID);


	AudioLayer * showMenuAndGetAudioLayer();
	AudioLayer* getAudioLayerForItemID(int itemID);

	TimeTrigger* showMenuAndGetTrigger();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceManager)
};