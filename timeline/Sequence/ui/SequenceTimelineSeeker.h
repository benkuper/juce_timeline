/*
  ==============================================================================

    SequenceTimelineSeeker.h
    Created: 23 Nov 2016 11:31:50pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once


class SeekHandle :
	public Component
{
public:
	SeekHandle() {}
	~SeekHandle() {}

	void paint(Graphics &g) override;
	void resized() override;
};

class SequenceTimelineSeeker :
	public Component,
	public ContainerAsyncListener
{
public:
	SequenceTimelineSeeker(Sequence * _sequence);
	~SequenceTimelineSeeker();

	Sequence * sequence;
	SeekHandle handle;

	//interaction
	Point<float> screenMousePosOnDown; 
	float viewStartAtMouseDown;
	float viewEndAtMouseDown;
	float timeAnchorAtMouseDown;
	float viewTimeAtMouseDown;
	const float minActionDistX = 10;
	const float minActionDistY = 15;
	const float zoomSensitivity = .002f;

	

	void paint(Graphics &g) override;
	void resized() override;

	void mouseDown(const MouseEvent &e) override;
	void mouseDrag(const MouseEvent &e) override;
	void mouseUp(const MouseEvent &e) override;

	int getXForTime(float time);
	float getTimeForX(int tx);

	void newMessage(const ContainerAsyncEvent &e) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceTimelineSeeker)

};