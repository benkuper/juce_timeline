/*
  ==============================================================================

    SequenceBlockUI.h
    Created: 8 Feb 2017 12:20:09pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class SequenceBlockUI :
	public LayerBlockUI
{
public:
	SequenceBlockUI(SequenceBlock * block);
	~SequenceBlockUI();

	SequenceBlock* block;

	void paint(Graphics &g) override;

	virtual void controllableFeedbackUpdateInternal(Controllable *) override;
};