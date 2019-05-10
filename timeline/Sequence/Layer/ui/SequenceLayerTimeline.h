/*
  ==============================================================================

    SequenceLayerTimeline.h
    Created: 28 Oct 2016 8:17:29pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceLayerTimeline :
	public BaseItemMinimalUI<SequenceLayer>
{
public:
	SequenceLayerTimeline(SequenceLayer *);
	virtual ~SequenceLayerTimeline();

	const Colour defaultTimeBarColor = Colours::white.withAlpha(.4f);
	Colour timeBarColor;
	int getXForTime(float time);
	float getTimeForX(int tx, bool offsetStart = true);

	virtual void updateContent() {} // to be overriden
	virtual void paintOverChildren(Graphics & g) override;
	virtual void mouseDown(const MouseEvent &e) override;

	SequenceLayerPanel * getRelatedPanel();

	void controllableFeedbackUpdateInternal(Controllable * c) override;



	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceLayerTimeline)
};