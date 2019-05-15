/*
  ==============================================================================

    SequenceTimelineNavigationUI.h
    Created: 20 Nov 2016 2:25:19pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class SequenceTimelineNavigationUI :
	public Component
{
public:
	SequenceTimelineNavigationUI(Sequence * _sequence, SequenceTimelineSeeker * seeker = nullptr, SequenceTimelineHeader * header = nullptr);
	~SequenceTimelineNavigationUI();

	Sequence * sequence;

	std::unique_ptr<SequenceTimelineSeeker> seeker;
	std::unique_ptr<SequenceTimelineHeader> header;

	void paint(Graphics &g) override;
	void resized() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceTimelineNavigationUI)
};