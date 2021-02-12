/*
  ==============================================================================

    SequenceBlockManagerUI.h
    Created: 8 Feb 2017 12:20:16pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class SequenceBlockLayerTimeline;

class SequenceBlockManagerUI :
	public LayerBlockManagerUI
{
public:
	SequenceBlockManagerUI(SequenceBlockLayerTimeline * timeline, SequenceBlockManager * manager);
	~SequenceBlockManagerUI();

	virtual LayerBlockUI* createUIForItem(LayerBlock* item) override;
};