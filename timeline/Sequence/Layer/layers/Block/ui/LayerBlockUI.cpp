#include "LayerBlockUI.h"
/*
  ==============================================================================

	LayerBlockUI.cpp
	Created: 14 Feb 2019 11:14:58am
	Author:  bkupe

  ==============================================================================
*/

LayerBlockUI::LayerBlockUI(LayerBlock * block) :
	BaseItemUI(block, ResizeMode::NONE, VERTICAL),
	viewStart(0),
	viewEnd(block->getTotalLength()),
	viewCoreEnd(block->coreLength->floatValue()),
	coreGrabber(Grabber::VERTICAL),
	loopGrabber(Grabber::VERTICAL)

{
	bgColor = BG_COLOR.brighter(item->isActive->boolValue() ? .1f : .05f);

	customDragBehavior = true;

	addAndMakeVisible(&coreGrabber);
	addAndMakeVisible(&loopGrabber);
}

LayerBlockUI::~LayerBlockUI()
{
}


void LayerBlockUI::paint(Graphics & g)
{
	if (inspectable.wasObjectDeleted()) return;

	BaseItemUI::paint(g);
	g.fillCheckerBoard(getMainBounds().withLeft(getCoreWidth()).toFloat(), 16,16, Colours::white.withAlpha(.05f), Colours::white.withAlpha(.1f));
}

void LayerBlockUI::resizedInternalHeader(Rectangle<int>& r)
{
	loopGrabber.setVisible(item->loopLength->floatValue() > 0);
	if(loopGrabber.isVisible())	loopGrabber.setBounds(r.removeFromRight(10));
	r.setRight(getCoreWidth());
	coreGrabber.setBounds(r.removeFromRight(10)); 
	r.removeFromRight(2);
	r.setRight(itemLabel.getX() + jmax(itemLabel.getFont().getStringWidth(itemLabel.getText()) + 5, 80));
}

void LayerBlockUI::resizedInternalContent(Rectangle<int>& r)
{
	Rectangle<int> lr(r);
}

void LayerBlockUI::mouseEnter(const MouseEvent & e)
{
	BaseItemUI::mouseEnter(e);
	grabber->setVisible(true);
	coreGrabber.setVisible(true);
	loopGrabber.setVisible(true && item->loopLength->floatValue() > 0);
}

void LayerBlockUI::mouseExit(const MouseEvent & e)
{
	BaseItemUI::mouseExit(e);
	grabber->setVisible(isMouseOverOrDragging());
	coreGrabber.setVisible(isMouseOverOrDragging());
	loopGrabber.setVisible(isMouseOverOrDragging() && item->loopLength->floatValue() > 0);
}


void LayerBlockUI::mouseDown(const MouseEvent & e)
{
	if(e.eventComponent != grabber) BaseItemUI::mouseDown(e);

	timeAtMouseDown = item->time->floatValue();
	coreLengthAtMouseDown = item->coreLength->floatValue();
	loopLengthAtMouseDown = item->loopLength->floatValue();

	posAtMouseDown = getX();
}

void LayerBlockUI::mouseDrag(const MouseEvent & e)
{

	 if (e.eventComponent == grabber)
	{
		blockUIListeners.call(&BlockUIListener::blockUIStartDragged, this, e);
	}
	else if (e.eventComponent == &coreGrabber)
	{
		blockUIListeners.call(&BlockUIListener::blockUICoreDragged, this, e);
	}
	else if (e.eventComponent == &loopGrabber)
	{
		blockUIListeners.call(&BlockUIListener::blockUILoopDragged, this, e);
	}
	else if (e.eventComponent == this && e.getPosition().getX() < getCoreWidth() && e.getPosition().getY() < headerHeight)
	{
		 blockUIListeners.call(&BlockUIListener::blockUIDragged, this, e);
	}

	 BaseItemUI::mouseDrag(e);

}

void LayerBlockUI::mouseUp(const MouseEvent & e)
{

	if (e.eventComponent == grabber)
	{
		item->time->setUndoableValue(timeAtMouseDown, item->time->floatValue());
		item->coreLength->setUndoableValue(coreLengthAtMouseDown, item->coreLength->floatValue());
	}
	else if (e.eventComponent == &coreGrabber)
	{
		item->time->setUndoableValue(timeAtMouseDown, item->time->floatValue());
		item->coreLength->setUndoableValue(coreLengthAtMouseDown, item->coreLength->floatValue());
	}
	else if (e.eventComponent == &loopGrabber)
	{
		item->loopLength->setUndoableValue(loopLengthAtMouseDown, item->loopLength->floatValue());
	}
	else if (e.eventComponent == this && e.getPosition().getX() < getCoreWidth() && e.getPosition().getY() < headerHeight)
	{
		item->time->setUndoableValue(timeAtMouseDown, item->time->floatValue());
		blockUIListeners.call(&BlockUIListener::blockUINeedsReorder);
	}	

	grabber->setVisible(isMouseOverOrDragging());
	coreGrabber.setVisible(isMouseOverOrDragging());
	loopGrabber.setVisible(isMouseOverOrDragging());

	BaseItemUI::mouseUp(e);
}


void LayerBlockUI::controllableFeedbackUpdateInternal(Controllable * c)
{
	if (c == item->time || c == item->coreLength || c == item->loopLength)
	{
		blockUIListeners.call(&BlockUIListener::blockUITimeChanged, this);
	}
	else if (c == item->isActive)
	{
		bgColor = BG_COLOR.brighter(item->isActive->boolValue() ? .1f : .05f);
	}
}

Rectangle<int> LayerBlockUI::getCoreBounds()
{
	return getLocalBounds().withWidth(getCoreWidth());
}

int LayerBlockUI::getCoreWidth()
{
	return (item->coreLength->floatValue() / item->getTotalLength()) * getMainBounds().getWidth();
}

void LayerBlockUI::setViewRange(float relativeStart, float relativeEnd)
{
	relativeStart = jmax<float>(relativeStart, 0);
	relativeEnd = jmin<float>(relativeEnd, item->getTotalLength());

	if (viewStart == relativeStart && viewEnd == relativeEnd) return;


	viewStart = relativeStart;
	viewEnd = relativeEnd;

	viewCoreEnd = jmin(viewEnd, item->coreLength->floatValue());
	setViewRangeInternal();
}

/*
LayerBlockUI::StretchHandle::StretchHandle(const Colour & c) :
	color(c)
{
	setRepaintsOnMouseActivity(true);
	setMouseCursor(MouseCursor::LeftEdgeResizeCursor);
}

LayerBlockUI::StretchHandle::~StretchHandle()
{
}

void LayerBlockUI::StretchHandle::paint(Graphics & g)
{
	g.setColour(isMouseOver() ? color : color.withAlpha(.3f));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);
}
*/