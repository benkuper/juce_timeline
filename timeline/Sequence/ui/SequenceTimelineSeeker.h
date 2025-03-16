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
};

class SeekNeedle :
	public Component
{
public:
	SeekNeedle() {}
	~SeekNeedle() {}

	void paint(Graphics &g) override;
};

class SequenceTimelineSeeker :
	public Component,
	public UITimerTarget,
	public ContainerAsyncListener
{
public:
	SequenceTimelineSeeker(Sequence * _sequence);
	~SequenceTimelineSeeker();

	Sequence * sequence;
	SeekHandle handle;
	SeekNeedle needle;

	//interaction
	Point<float> screenMousePosOnDown; 
	float viewStartAtMouseDown;
	float viewEndAtMouseDown;
	float timeAnchorAtMouseDown;
	float viewTimeAtMouseDown;
	const float minActionDistX = 10;
	const float minActionDistY = 15;
	const float zoomSensitivity = .002f;

	Point<float> selectionSpan;


	void paint(Graphics &g) override;
	void resized() override;
	void handlePaintTimerInternal() override;
    void paintOverChildren(juce::Graphics& g) override;

	void mouseDown(const MouseEvent &e) override;
	void mouseDrag(const MouseEvent &e) override;
	void mouseUp(const MouseEvent &e) override;

	int getXForTime(float time);
	float getTimeForX(int tx);

	void newMessage(const ContainerAsyncEvent &e) override;

	class SeekerListener
	{
	public:
		virtual ~SeekerListener() {}
		virtual void seekerManipulationChanged(bool isManipulating) {}
	};

	ListenerList<SeekerListener> seekerListeners;
	void addSeekerListener(SeekerListener* newListener) { seekerListeners.add(newListener); }
	void removeSeekerListener(SeekerListener* listener) { seekerListeners.remove(listener); }

};