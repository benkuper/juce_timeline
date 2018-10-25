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
	juce_DeclareSingleton(SequenceManager, false)

	SequenceManager();
	~SequenceManager();

	static Sequence * showMenuAndGetSequence();
	Sequence * getSequenceForItemID(int itemID);

	static SequenceLayer * showmMenuAndGetLayer();
	SequenceLayer * getLayerForItemID(int itemID);

	static TimeCue * showMenuAndGetCue();
	TimeCue * getCueForItemID(int itemID);


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceManager)
};