/*
  ==============================================================================

    SequenceTimelineHeader.h
    Created: 23 Nov 2016 11:32:15pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class TimeNeedleUI :
	public Component
{
public:
	TimeNeedleUI() {}
	~TimeNeedleUI() {}

	void paint(Graphics &g) override;
};

class SequenceTimelineHeader :
	public Component,
	public ContainerAsyncListener,
    public Timer
{
public:
	SequenceTimelineHeader(Sequence * _sequence, TimeCueManagerUI * curManagerUI = nullptr, TimeNeedleUI * needleUI = nullptr);
	~SequenceTimelineHeader();

	Sequence * sequence;

	std::unique_ptr<TimeNeedleUI> needle;
	std::unique_ptr<TimeCueManagerUI> cueManagerUI;

    bool shouldUpdateNeedle;

	bool selectionZoomMode;
	Point<float> selectionSpan;
    
	//Interaction

	void paint(Graphics &g) override;
	void resized() override;

	void updateNeedlePosition();

	void mouseDown(const MouseEvent &e) override;
	void mouseDrag(const MouseEvent &e) override;
	void mouseDoubleClick(const MouseEvent &e) override;
	void mouseUp(const MouseEvent& e) override;

	int getXForTime(float time);
	float getTimeForX(int tx);

	void newMessage(const ContainerAsyncEvent &e) override;

    void timerCallback() override;
	
	
};