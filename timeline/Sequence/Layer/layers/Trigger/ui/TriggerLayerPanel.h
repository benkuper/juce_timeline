/*
  ==============================================================================

    TriggerLayerPanel.h
    Created: 20 Nov 2016 3:07:50pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class TriggerLayerPanel :
	public SequenceLayerPanel
{
public:
	TriggerLayerPanel(TriggerLayer * layer);
	~TriggerLayerPanel();
	
	TriggerLayer * triggerLayer;
	ScopedPointer<TriggerButtonUI> lockAllBT;
	ScopedPointer<TriggerButtonUI> unlockAllBT;

	void resizedInternalContent(Rectangle<int> &r) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerLayerPanel)
};