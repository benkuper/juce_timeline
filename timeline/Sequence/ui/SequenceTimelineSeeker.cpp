/*
  ==============================================================================

    SequenceTimelineSeeker.cpp
    Created: 23 Nov 2016 11:31:50pm
    Author:  Ben Kuper

  ==============================================================================
*/

#include "JuceHeader.h"

SequenceTimelineSeeker::SequenceTimelineSeeker(Sequence * _sequence) :
	UITimerTarget(ORGANICUI_SLOW_TIMER, "SequenceTimelineSeeker"),
	sequence(_sequence)
{
	setBufferedToImage(true);
	handle.setBufferedToImage(true);

	addAndMakeVisible(&needle);
	addAndMakeVisible(&handle);
	sequence->addAsyncContainerListener(this);
	handle.addMouseListener(this,false);
}

SequenceTimelineSeeker::~SequenceTimelineSeeker()
{
	sequence->removeAsyncContainerListener(this);
}

#pragma warning (push)
#pragma warning(disable:4244)
void SequenceTimelineSeeker::paint(Graphics & g)
{
	g.setColour(BG_COLOR);
	g.fillRoundedRectangle(getLocalBounds().toFloat(),2);
	g.setColour(BG_COLOR.darker());
	g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 2);

	if (!selectionSpan.isOrigin())
	{
		Colour c = BLUE_COLOR;
		float minPos = jmin(getXForTime(selectionSpan.x), getXForTime(selectionSpan.y));
		float maxPos = jmax(getXForTime(selectionSpan.x), getXForTime(selectionSpan.y));
		Rectangle<int> sr = Rectangle<int>(minPos, 0, maxPos - minPos, getHeight()).reduced(1);
		g.setColour(c.withAlpha(.3f));
		g.fillRect(sr);
		g.setColour(c.withAlpha(.7f));
		g.drawRect(sr);
	}
}

void SequenceTimelineSeeker::resized()
{
	shouldRepaint = true;
}

void SequenceTimelineSeeker::handlePaintTimerInternal()
{
	float tx = getXForTime(sequence->currentTime->floatValue());
	needle.setBounds(tx, 0, 2, getHeight());

	Rectangle<int> r = getLocalBounds();
	r.setLeft(getXForTime(sequence->viewStartTime->floatValue()));
	r.setRight(getXForTime(sequence->viewEndTime->floatValue()));
	handle.setBounds(r);
}

void SequenceTimelineSeeker::paintOverChildren(Graphics& g)
{
	validatePaint();
}

void SequenceTimelineSeeker::mouseDown(const MouseEvent & e)
{
	if (e.mods.isRightButtonDown())
	{
		float pos = getTimeForX(e.getEventRelativeTo(this).getPosition().x);
		selectionSpan.setXY(pos, pos);
	}
	else if (e.mods.isLeftButtonDown())
	{
		viewStartAtMouseDown = sequence->viewStartTime->floatValue();
		viewEndAtMouseDown = sequence->viewEndTime->floatValue();
		timeAnchorAtMouseDown = getTimeForX(e.getEventRelativeTo(this).x);
		viewTimeAtMouseDown = (viewEndAtMouseDown - viewStartAtMouseDown);
	}

	e.source.enableUnboundedMouseMovement(true, false);
	setMouseCursor(MouseCursor::NoCursor);
	seekerListeners.call(&SeekerListener::seekerManipulationChanged, true);

	//DBG("viewStartAtMouseDown : " << viewStartAtMouseDown << "/ viewEndAtMouseDown : " << viewEndAtMouseDown << "/ timeAnchorAtMouseDown : " << timeAnchorAtMouseDown);
}


void SequenceTimelineSeeker::mouseDrag(const MouseEvent & e)
{
	if (e.mods.isRightButtonDown())
	{
		float pos = getTimeForX(e.getEventRelativeTo(this).getPosition().x);
		selectionSpan.setY(pos);
		repaint();
	}
	else if (e.mods.isLeftButtonDown())
	{
		bool followPlayingMode = sequence->isPlaying->boolValue() && sequence->viewFollowTime->boolValue();

		if (e.originalComponent == &handle)
		{

			if (e.getScreenPosition().x == 0 || e.getScreenPosition().y == 0) return;

			Point<float> offset = (e.getScreenPosition() - e.getMouseDownScreenPosition()).toFloat();
			//float offsetY = e.getEventRelativeTo(this).getOffsetFromDragStart().y;

			offset.x = offset.x < 0 ? jmin<float>(offset.x + minActionDistX, 0) : jmax<float>(offset.x - minActionDistX, 0);
			offset.y = offset.y< 0 ? jmin<float>(offset.y + minActionDistY, 0) : jmax<float>(offset.y - minActionDistY, 0);

			offset.y *= sequence->totalTime->floatValue();
			if (e.mods.isShiftDown()) offset.y *= 2;
			if (e.mods.isAltDown()) offset.y /= 2;
			offset.y *= zoomSensitivity;

			float viewTimeFactor = ((viewTimeAtMouseDown - offset.y) / viewTimeAtMouseDown);

			if (viewTimeFactor == 0) return;

			float startDist = timeAnchorAtMouseDown - viewStartAtMouseDown;
			float endDist = timeAnchorAtMouseDown - viewEndAtMouseDown;
			float newViewStart = timeAnchorAtMouseDown - startDist*viewTimeFactor + getTimeForX(offset.x);
			float newViewEnd = timeAnchorAtMouseDown - endDist*viewTimeFactor + getTimeForX(offset.x);

			float newViewRange = newViewEnd - newViewStart;
			
			if (newViewRange >= sequence->minViewTime->floatValue() || fabsf(offset.x) > 20)
			{
				if (followPlayingMode) sequence->followViewRange = newViewEnd - newViewStart;
				else
				{
					sequence->viewStartTime->setValue(newViewStart);
					sequence->viewEndTime->setValue(newViewEnd);
				}
			}
		}
		else
		{
			if (!followPlayingMode)
			{
				float destTime = getTimeForX(e.getPosition().x);
				sequence->viewStartTime->setValue(destTime - viewTimeAtMouseDown / 2);
				sequence->viewEndTime->setValue(destTime + viewTimeAtMouseDown / 2);
			}
			
		}
	}
}

void SequenceTimelineSeeker::mouseUp(const MouseEvent & e)
{
	if (e.mods.isRightButtonDown())
	{
		float start = 0;
		float end = sequence->totalTime->floatValue();

		if (e.mouseWasDraggedSinceMouseDown())
		{
			start = jmin(selectionSpan.x, selectionSpan.y);
			end = jmax(selectionSpan.x, selectionSpan.y);
		}

		selectionSpan.setXY(0, 0);

		bool followPlayingMode = sequence->isPlaying->boolValue() && sequence->viewFollowTime->boolValue();
		if (followPlayingMode) sequence->followViewRange = end - start;
		else
		{
			sequence->viewStartTime->setValue(start);
			sequence->viewEndTime->setValue(end);
		}

	}
	
	setMouseCursor(MouseCursor::NormalCursor);
	e.source.enableUnboundedMouseMovement(false, true);

	seekerListeners.call(&SeekerListener::seekerManipulationChanged, false);
}


int SequenceTimelineSeeker::getXForTime(float time)
{
	return (int)jmap<float>(time, 0, sequence->totalTime->floatValue(), 0, (float)getWidth());
}

float SequenceTimelineSeeker::getTimeForX(int tx)
{
	return jmap<float>((float)tx, 0, (float)getWidth(), 0, sequence->totalTime->floatValue());
}


void SequenceTimelineSeeker::newMessage(const ContainerAsyncEvent & e)
{
	switch (e.type)
	{
	case ContainerAsyncEvent::ControllableFeedbackUpdate:

		if (e.targetControllable.wasObjectDeleted()) return;
		if (e.targetControllable == sequence->viewStartTime || e.targetControllable == sequence->viewEndTime)
		{
			shouldRepaint = true;
		}
		else if (e.targetControllable == sequence->currentTime)
		{
			shouldRepaint = true;
		}
		else if (e.targetControllable == sequence->totalTime)
		{
			repaint();
			shouldRepaint = true;
		}
		break;

	default:
		//other events not handled 
		break;
	}
}


void SeekHandle::paint(Graphics & g)
{
	g.setColour(BLUE_COLOR.withAlpha(.3f));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);
	g.setColour(BLUE_COLOR.brighter());
	g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 2);
}

void SeekNeedle::paint(Graphics & g)
{
	g.setColour(HIGHLIGHT_COLOR);
	g.drawLine(0, 0, 0, getHeight(), 2);
}

#pragma warning (pop)