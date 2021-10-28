/*
  ==============================================================================

	LayerBlockUI.cpp
	Created: 14 Feb 2019 11:14:58am
	Author:  bkupe

  ==============================================================================
*/
LayerBlockUI::LayerBlockUI(LayerBlock * block) :
	BaseItemMinimalUI(block),
	viewStart(0),
	viewEnd(block->getTotalLength()),
	viewCoreEnd(block->coreLength->floatValue()),
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


void LayerBlockUI::paint(Graphics & g)
{
	if (inspectable.wasObjectDeleted()) return;

	BaseItemMinimalUI::paint(g);
	g.fillCheckerBoard(getMainBounds().withLeft(getCoreWidth()).toFloat(), 16, 16, Colours::white.withAlpha(.05f), Colours::white.withAlpha(.1f));
}

void LayerBlockUI::paintOverChildren(Graphics& g)
{
	if (inspectable.wasObjectDeleted()) return;
	BaseItemMinimalUI::paintOverChildren(g);

	if (item->isUILocked->boolValue())
	{
		g.setTiledImageFill(ImageCache::getFromMemory(TimelineBinaryData::stripe_png, TimelineBinaryData::stripe_pngSize), 0, 0, .1f);
		g.fillAll();
	}
}

void LayerBlockUI::resized()
{
	if (canBeGrabbed)
	{
		Rectangle<int> r = getGrabberBounds();

		const int grabberSize = 9;

		grabber.setBounds(r.removeFromLeft(grabberSize));

		loopGrabber.setVisible(item->loopLength->floatValue() > 0);
		if (loopGrabber.isVisible())	loopGrabber.setBounds(r.removeFromRight(grabberSize));

		r.setRight(getCoreWidth());
		coreGrabber.setBounds(r.removeFromRight(grabberSize));
	}

	resizedBlockInternal();
}

void LayerBlockUI::mouseEnter(const MouseEvent & e)
{
	BaseItemMinimalUI::mouseEnter(e);

	if (inspectable.wasObjectDeleted()) return;


	if (canBeGrabbed && !item->isUILocked->boolValue() && getWidth() > 24)
	{
		grabber.setVisible(true);
		coreGrabber.setVisible(true);
		loopGrabber.setVisible(true && item->loopLength->floatValue() > 0);
	}
}

void LayerBlockUI::mouseExit(const MouseEvent & e)
{
	BaseItemMinimalUI::mouseExit(e);
	
	if (canBeGrabbed)
	{
		grabber.setVisible(isMouseOverOrDragging());
		coreGrabber.setVisible(isMouseOverOrDragging());
		loopGrabber.setVisible(isMouseOverOrDragging() && item->loopLength->floatValue() > 0);

		if (isMouseOverOrDragging())
		{
			grabber.toFront(false);
			coreGrabber.toFront(false);
			loopGrabber.toFront(false);
		}
	}
}


void LayerBlockUI::mouseDown(const MouseEvent & e)
{
	BaseItemMinimalUI::mouseDown(e);

	if (canBeGrabbed && !item->isUILocked->boolValue())
	{
		item->setMovePositionReference(true);
		coreLengthAtMouseDown = item->coreLength->floatValue();
		loopLengthAtMouseDown = item->loopLength->floatValue();

		isDragging = e.eventComponent == this && getDragBounds().contains(e.getPosition()) && !e.mods.isCommandDown() && !e.mods.isShiftDown();
		posAtMouseDown = getX();

		blockUIListeners.call(&BlockUIListener::blockUIMouseDown, this, e);
	}
}

void LayerBlockUI::mouseDrag(const MouseEvent & e)
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
	BaseItemMinimalUI::mouseDrag(e);

}

void LayerBlockUI::mouseUp(const MouseEvent & e)
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
			item->time->setUndoableValue(item->movePositionReference.x, item->time->floatValue());
			item->coreLength->setUndoableValue(coreLengthAtMouseDown, item->coreLength->floatValue());
		}
		else if (e.eventComponent == &coreGrabber)
		{
			item->time->setUndoableValue(item->movePositionReference.x, item->time->floatValue());
			item->coreLength->setUndoableValue(coreLengthAtMouseDown, item->coreLength->floatValue());
		}
		else if (e.eventComponent == &loopGrabber)
		{
			item->loopLength->setUndoableValue(loopLengthAtMouseDown, item->loopLength->floatValue());
		}

		isDragging = false;

		grabber.setVisible(isMouseOverOrDragging());
		coreGrabber.setVisible(isMouseOverOrDragging());
		loopGrabber.setVisible(isMouseOverOrDragging());
	}

	BaseItemMinimalUI::mouseUp(e);
}

Rectangle<int> LayerBlockUI::getDragBounds()
{

	return getLocalBounds();
}

Rectangle<int> LayerBlockUI::getGrabberBounds()
{
	return getLocalBounds().reduced(0, 8);
}


void LayerBlockUI::controllableFeedbackUpdateInternal(Controllable * c)
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
