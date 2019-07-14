/*
  ==============================================================================

    SequenceLayerTimelineManagerUI.h
    Created: 20 Nov 2016 2:18:18pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class SequenceLayerTimelineManagerUI :
	public BaseManagerUI<SequenceLayerManager, SequenceLayer, SequenceLayerTimeline>
{
public:

	SequenceLayerTimelineManagerUI(SequenceLayerManager *_manager);
	~SequenceLayerTimelineManagerUI();

	SequenceLayerTimeline * createUIForItem(SequenceLayer * layer) override;

	void resized() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceLayerTimelineManagerUI)
};