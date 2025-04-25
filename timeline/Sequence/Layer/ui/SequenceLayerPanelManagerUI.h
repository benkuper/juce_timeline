/*
  ==============================================================================

    SequenceLayerPanelManagerUI.h
    Created: 28 Oct 2016 8:15:41pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceLayerPanelManagerUI :
	public ManagerUI<SequenceLayerManager, SequenceLayer, SequenceLayerPanel, ItemBaseGroup<SequenceLayer>, SequenceLayerGroupPanel>
{
public:

	SequenceLayerPanelManagerUI(SequenceLayerManager* _manager, bool useViewport = true);
	~SequenceLayerPanelManagerUI();

	SequenceLayerPanel * createUIForItem(SequenceLayer * layer) override;

    void addItemFromMenu(SequenceLayer* item, bool, Point<int>) override;
};