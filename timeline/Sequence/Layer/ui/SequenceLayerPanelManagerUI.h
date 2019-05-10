/*
  ==============================================================================

    SequenceLayerPanelManagerUI.h
    Created: 28 Oct 2016 8:15:41pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceLayerPanelManagerUI :
	public BaseManagerUI<SequenceLayerManager, SequenceLayer, SequenceLayerPanel>
{
public:

	SequenceLayerPanelManagerUI(SequenceLayerManager *_manager);
	~SequenceLayerPanelManagerUI();

	SequenceLayerPanel * createUIForItem(SequenceLayer * layer) override;
	void showMenuAndAddItem(bool isFromAddButton, Point<int>) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceLayerPanelManagerUI)
};