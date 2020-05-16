/*
  ==============================================================================

    SequenceTransportUI.h
    Created: 20 Nov 2016 2:24:35pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class SequenceTransportUI :
	public Component,
	public Sequence::SequenceListener
{
public:
	SequenceTransportUI(Sequence * _sequence);
	~SequenceTransportUI();

	Sequence * sequence;
	TimeLabel timeLabel;
	TimeLabel timeStepLabel;
	TimeLabel totalTimeLabel;
	
	std::unique_ptr<TriggerImageUI> togglePlayUI;
	std::unique_ptr<TriggerImageUI> stopUI;
	std::unique_ptr<TriggerImageUI> prevCueUI;
	std::unique_ptr<TriggerImageUI> nextCueUI;
	std::unique_ptr<BoolImageToggleUI> loopUI;
	
	void paint(Graphics &g) override;
	void resized() override;

	void sequenceMasterAudioModuleChanged(Sequence *) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceTransportUI)
};