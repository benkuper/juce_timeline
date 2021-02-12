/*
  ==============================================================================

    SequenceBlockLayerPanel.h
    Created: 20 Nov 2016 3:08:49pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class SequenceBlockLayerPanel :
	public SequenceLayerPanel
{
public:
	SequenceBlockLayerPanel(SequenceBlockLayer * layer);
	~SequenceBlockLayerPanel();

	SequenceBlockLayer * sequenceBlockLayer;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceBlockLayerPanel)
};