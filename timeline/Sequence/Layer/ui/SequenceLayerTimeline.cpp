/*
  ==============================================================================

    SequenceLayerTimeline.cpp
    Created: 28 Oct 2016 8:17:29pm
    Author:  bkupe

  ==============================================================================
*/

SequenceLayerTimeline::SequenceLayerTimeline(SequenceLayer * layer) :
	BaseItemMinimalUI<SequenceLayer>(layer),
	UITimerTarget(ORGANICUI_DEFAULT_TIMER, "SequenceLayerTimeline"),
	seekManipulationMode(false)
{
	bgColor = item->itemColor->getColor();

	item->sequence->addAsyncContainerListener(this);
	setSize(100, item->uiHeight->intValue());

	bringToFrontOnSelect = false;
	//setInterceptsMouseClicks(true, true);
	setWantsKeyboardFocus(false);
	setMouseClickGrabsKeyboardFocus(false);
	autoSelectWithChildRespect = false;
	autoDrawContourWhenSelected = false;

    addAndMakeVisible(&needle);
}

SequenceLayerTimeline::~SequenceLayerTimeline()
{
	if (inspectable.wasObjectDeleted()) return;
	item->sequence->removeAsyncContainerListener(this);
}


int SequenceLayerTimeline::getXForTime(float time)
{
	float viewStart = item->sequence->viewStartTime->floatValue();
	float viewEnd = item->sequence->viewEndTime->floatValue();
	if (viewStart == viewEnd) return 0;
	return (int)jmap<float>(time, viewStart, viewEnd, 0, (float)getWidth());
}

float SequenceLayerTimeline::getTimeForX(int tx, bool offsetStart)
{
	if (getWidth() == 0) return 0; 
	
	float viewStart = item->sequence->viewStartTime->floatValue();
	float viewEnd = item->sequence->viewEndTime->floatValue();
	float viewTime = viewEnd - viewStart;
	float mapStart = offsetStart ? viewStart : 0;
	return item->sequence->currentTime->getStepSnappedValueFor(jmap<float>((float)tx, 0, (float)getWidth(), mapStart, mapStart + viewTime));
}



void SequenceLayerTimeline::updateNeedlePosition()
{
    int tx = getXForTime(item->sequence->currentTime->floatValue());
    needle.setBounds(getLocalBounds().withWidth(1).withX(tx));
	needle.toFront(false);
}

void SequenceLayerTimeline::mouseDown(const MouseEvent &e)
{
	BaseItemMinimalUI::mouseDown(e);
	/*
	if (e.mods.isLeftButtonDown() && e.eventComponent->getParentComponent() == this)
	{
		if(e.mods.isShiftDown()) item->selectThis(true);
		else
		{
			if (!item->isSelected) item->selectThis(e.mods.isCommandDown());
			else if (e.mods.isCommandDown()) item->deselectThis();
		}
	}
	*/
}

void SequenceLayerTimeline::setSeekManipulationMode(bool isManipulating)
{
	seekManipulationMode = isManipulating;
}

void SequenceLayerTimeline::controllableFeedbackUpdateInternal(Controllable * c)
{
	BaseItemMinimalUI::controllableFeedbackUpdateInternal(c);

	if (c == item->uiHeight)
	{
		setSize(getWidth(), item->uiHeight->intValue());
	}
	else if (c == item->sequence->viewStartTime || c == item->sequence->viewEndTime)
	{
		if (isVisible())
		{
			updateContent();
			shouldRepaint = true;
		}
	}
	else if (c == item->sequence->currentTime)
	{
		shouldRepaint = true;
	}
	else if (c == item->miniMode)
	{
		updateMiniModeUI();
	}
}

void SequenceLayerTimeline::handlePaintTimerInternal()
{
	updateNeedlePosition();
}

void SequenceLayerTimeline::visibilityChanged()
{
	if (!isVisible()) return;

	updateContent();
	shouldRepaint = true;
}

void SequenceLayerTimeline::parentSizeChanged()
{
	shouldRepaint = true;
}

SequenceLayerTimeline::TimelineNeedle::TimelineNeedle() :
timeBarColor(defaultTimeBarColor)
{
    
}

SequenceLayerTimeline::TimelineNeedle::~TimelineNeedle()
{
    
}

void SequenceLayerTimeline::TimelineNeedle::paint(juce::Graphics &g)
{
    g.fillAll(timeBarColor);
}