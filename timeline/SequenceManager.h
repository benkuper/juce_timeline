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

class SequenceManager :
	public BaseManager<Sequence>
{
public:
#if TIMELINE_USE_SEQUENCEMANAGER_SINGLETON
	juce_DeclareSingleton(SequenceManager, false)
#endif

	SequenceManager();
	~SequenceManager();

	Factory<SequenceLayer>* defaultLayerFactory;

	virtual void addItemInternal(Sequence* item, var data) override;

	Sequence * showMenuAndGetSequence();
	Sequence * getSequenceForItemID(int itemID);

	SequenceLayer * showmMenuAndGetLayer();
	SequenceLayer * getLayerForItemID(int itemID);

	TimeCue * showMenuAndGetCue();
	TimeCue * getCueForItemID(int itemID);


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceManager)
};