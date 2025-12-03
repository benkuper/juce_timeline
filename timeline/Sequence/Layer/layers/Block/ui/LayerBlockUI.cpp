/*
  ==============================================================================

	LayerBlockUI.cpp
	Created: 14 Feb 2019 11:14:58am
	Author:  bkupe

  ==============================================================================
*/

#include "JuceHeader.h"

LayerBlockUI::LayerBlockUI(LayerBlock* block) :
	ItemMinimalUI(block),
	UITimerTarget(ORGANICUI_SLOW_TIMER, "LayerBlockUI"),
	blockManagerUI(nullptr),
	viewStart(0),
	viewEnd(block->getTotalLength()),
	viewCoreLength(block->coreLength->floatValue()),
	canBeGrabbed(true),
	baseColor(BG_COLOR.brighter(.1f)),
	highlightColor(BG_COLOR.brighter(.05f)),
	grabber(Grabber::VERTICAL),
	coreGrabber(Grabber::VERTICAL),
	loopGrabber(Grabber::VERTICAL)
{
	dragAndDropEnabled = false;

	bgColor = baseColor;

	if (canBeGrabbed)
	{
		addChildComponent(&grabber);
		addChildComponent(&coreGrabber);
		addChildComponent(&loopGrabber);
	}
}

LayerBlockUI::~LayerBlockUI()
{
}


void LayerBlockUI::paint(Graphics& g)
{
	if (inspectable.wasObjectDeleted()) return;

	ItemMinimalUI::paint(g);
	g.fillCheckerBoard(getMainBounds().withLeft(getCoreWidth()).toFloat(), 16, 16, Colours::white.withAlpha(.05f), Colours::white.withAlpha(.1f));
}

void LayerBlockUI::paintOverChildren(Graphics& g)
{
	if (inspectable.wasObjectDeleted()) return;
	ItemMinimalUI::paintOverChildren(g);

	if (item->isUILocked->boolValue())
	{
		g.setTiledImageFill(ImageCache::getFromMemory(TimelineBinaryData::stripe_png, TimelineBinaryData::stripe_pngSize), 0, 0, .1f);
		g.fillAll();
	}

	validatePaint();
}

void LayerBlockUI::handlePaintTimerInternal()
{
	repaint();
}

void LayerBlockUI::resized()
{
	if (blockManagerUI == nullptr) return;
	updateGrabbers();
	resizedBlockInternal();
}

void LayerBlockUI::updateGrabbers()
{
	if (inspectable.wasObjectDeleted()) return;

	if (!canBeGrabbed || !isMouseOverOrDragging(true) || getWidth() < 20 || item->isUILocked->boolValue())
	{
		grabber.setVisible(false);
		coreGrabber.setVisible(false);
		loopGrabber.setVisible(false);
		return;
	}

	int unclippedStart = blockManagerUI->timeline->getXForTime(item->time->floatValue());
	int unclippedCoreEnd = blockManagerUI->timeline->getXForTime(item->getCoreEndTime());
	int unclippedEnd = blockManagerUI->timeline->getXForTime(item->getEndTime());
	Rectangle<int> unclippedCoreBounds = Rectangle<int>(
		unclippedStart,
		0,
		unclippedCoreEnd - unclippedStart,
		getHeight());

	const int grabberSize = 9;

	Rectangle<int> grabberBounds = unclippedCoreBounds.removeFromLeft(grabberSize);
	Rectangle<int> coreGrabberBounds = unclippedCoreBounds.removeFromRight(grabberSize);

	grabber.setVisible(grabberBounds.getRight() > 0 && grabberBounds.getX() < getRight());

	if (grabber.isVisible()) grabber.setBounds(grabberBounds.translated(-getX(), 0));

	coreGrabber.setVisible(coreGrabberBounds.getRight() > 0 && coreGrabberBounds.getX() < getRight());
	if (coreGrabber.isVisible()) coreGrabber.setBounds(coreGrabberBounds.translated(-getX(), 0));


	Rectangle<int> unclippedLoopBounds = Rectangle<int>(
		unclippedCoreEnd,
		0,
		unclippedEnd - unclippedCoreEnd,
		getHeight());
	Rectangle<int> loopGrabberBounds = unclippedLoopBounds.removeFromRight(grabberSize);

	loopGrabber.setVisible(loopGrabberBounds.getRight() > 0 && loopGrabberBounds.getX() < getRight());
	if (loopGrabber.isVisible()) loopGrabber.setBounds(loopGrabberBounds.translated(-getX(), 0));
}

void LayerBlockUI::mouseEnter(const MouseEvent& e)
{
	ItemMinimalUI::mouseEnter(e);
	updateGrabbers();
}

void LayerBlockUI::mouseExit(const MouseEvent& e)
{
	ItemMinimalUI::mouseExit(e);
	updateGrabbers();
}


void LayerBlockUI::mouseDown(const MouseEvent& e)
{
	ItemMinimalUI::mouseDown(e);

	if (canBeGrabbed && !item->isUILocked->boolValue())
	{
		item->setMovePositionReference(true);
		coreLengthAtMouseDown = item->coreLength->floatValue();
		loopLengthAtMouseDown = item->loopLength->floatValue();

		isDragging = e.eventComponent == this && !e.mods.isCommandDown() && !e.mods.isShiftDown();
		posAtMouseDown = getX();

		blockUIListeners.call(&BlockUIListener::blockUIMouseDown, this, e);
	}
}

void LayerBlockUI::mouseDrag(const MouseEvent& e)
{
	if (canBeGrabbed && !item->isUILocked->boolValue())
	{
		if (isDragging)
		{
			blockUIListeners.call(&BlockUIListener::blockUIDragged, this, e);
		}
		else if (e.eventComponent == &grabber)
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
	}
	ItemMinimalUI::mouseDrag(e);

}

void LayerBlockUI::mouseUp(const MouseEvent& e)
{

	if (canBeGrabbed && !item->isUILocked->boolValue())
	{
		if (isDragging)
		{
			item->addMoveToUndoManager(true);
			blockUIListeners.call(&BlockUIListener::blockUINeedsReorder);
		}
		else if (e.eventComponent == &grabber)
		{
			item->time->setUndoableValue(item->time->floatValue());
			item->coreLength->setUndoableValue(item->coreLength->floatValue());
		}
		else if (e.eventComponent == &coreGrabber)
		{
			item->time->setUndoableValue(item->time->floatValue());
			item->coreLength->setUndoableValue(item->coreLength->floatValue());
		}
		else if (e.eventComponent == &loopGrabber)
		{
			item->loopLength->setUndoableValue(item->loopLength->floatValue());
		}

		isDragging = false;

		updateGrabbers();
	}

	ItemMinimalUI::mouseUp(e);
}



void LayerBlockUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	if (c == item->time || c == item->coreLength || c == item->loopLength)
	{
		blockUIListeners.call(&BlockUIListener::blockUITimeChanged, this);
	}
	else if (c == item->isActive)
	{
		bgColor = item->isActive->boolValue() ? baseColor : highlightColor;
	}
	else if (c == item->isUILocked)
	{
		repaint();
	}
}

Rectangle<int> LayerBlockUI::getCoreBounds()
{
	if (blockManagerUI == nullptr) return Rectangle<int>();
	return Rectangle<int>(0, 0, getCoreWidth(), getHeight());
}

int LayerBlockUI::getCoreWidth()
{
	if (blockManagerUI == nullptr) return 0;
	int startInView = blockManagerUI->timeline->getXForTime(viewStart);
	int coreEndInView = blockManagerUI->timeline->getXForTime(viewStart + viewCoreLength);
	return coreEndInView - startInView;
}

Rectangle<int> LayerBlockUI::getLoopBounds()
{
	return getLocalBounds().withLeft(getCoreWidth());

}
int LayerBlockUI::getRealXForTime(float time, bool relative)
{
	if (blockManagerUI == nullptr || inspectable.wasObjectDeleted()) return 0;
	float t = time;
	if (relative) t += item->time->floatValue();
	return blockManagerUI->timeline->getXForTime(t) - getX();
}

float LayerBlockUI::getTimeForX(int x, bool relative)
{
	if (blockManagerUI == nullptr || inspectable.wasObjectDeleted()) return 0;
	float mapStart = relative ? viewStart : 0;
	float mapEnd = relative ? viewEnd : item->getTotalLength();
	return jmap<float>((float)x, 0, (float)getWidth(), mapStart, mapEnd);
}

void LayerBlockUI::setViewRange(float relativeStart, float relativeEnd)
{
	relativeStart = jmax<float>(relativeStart, 0);
	relativeEnd = jmin<float>(relativeEnd, item->getTotalLength());

	if (viewStart == relativeStart && viewEnd == relativeEnd) return;


	viewStart = relativeStart;
	viewEnd = relativeEnd;

	viewCoreLength = jmax(0.f, jmin(viewEnd, item->coreLength->floatValue()) - viewStart);

	setViewRangeInternal();

	resized();
	shouldRepaint = true;
}
