/*
  ==============================================================================

	SequenceEditor.cpp
	Created: 28 Oct 2016 8:27:18pm
	Author:  bkupe

  ==============================================================================
*/

#include "JuceHeader.h"

SequenceEditorView::SequenceEditorView(Sequence* _sequence, SequenceTimelineNavigationUI* navigationUI, SequenceTransportUI* transportUI) :
	sequence(_sequence),
	sequenceRef(_sequence),
	navigationUI(navigationUI),
	panelManagerUI(_sequence->layerManager.get()),
	timelineManagerUI(_sequence->layerManager.get()),
	transportUI(transportUI),
	grabber(GapGrabber::HORIZONTAL),
	panelWidth(250)
{
	addAndMakeVisible(panelContainer);
	addAndMakeVisible(timelineContainer);

	if (this->transportUI == nullptr) this->transportUI.reset(new SequenceTransportUI(sequence));
	panelContainer.addAndMakeVisible(this->transportUI.get());
	panelContainer.addAndMakeVisible(&panelManagerUI);

	if (this->navigationUI == nullptr) this->navigationUI.reset(new SequenceTimelineNavigationUI(sequence));
	this->navigationUI->seeker->addSeekerListener(this);
	timelineContainer.addAndMakeVisible(this->navigationUI.get());
	timelineContainer.addAndMakeVisible(&timelineManagerUI);

	panelManagerUI.viewport.getVerticalScrollBar().addListener(this);
	timelineManagerUI.viewport.getVerticalScrollBar().addListener(this);

	panelManagerUI.viewport.addMouseListener(this, false);
	timelineManagerUI.viewport.addMouseListener(this, false);
	addMouseListener(this, true);

	sequence->setBeingEdited(true);
	sequence->addAsyncContainerListener(this);

	addAndMakeVisible(&grabber);
	grabber.addGrabberListener(this);
}

SequenceEditorView::~SequenceEditorView()
{
	//@Tom : on linux in Release mode (-O3), sequenceRef doesn't advertise as wasObjectDeleted() 
	//but sequence pointer is still fucked up (debuggin sequence->currentTime->floatvalue() will crash)
	//it's still showing "isClearing" as 1 so we can use that but's it's not proper
	if (!sequenceRef.wasObjectDeleted() && !sequence->isClearing)
	{
		sequence->removeAsyncContainerListener(this);
		sequence->setBeingEdited(false);
	}
}

void SequenceEditorView::paint(Graphics&)
{
	//g.fillAll(Colours::red);
	//g.setColour(Colours::yellow);
	//g.drawLine(panelWidth + 1, 0, panelWidth + 1, getHeight(), 1);
}

void SequenceEditorView::resized()
{
	Rectangle<int> r = getLocalBounds();

	if (sequenceRef.wasObjectDeleted() || sequence == nullptr) return;

	headerHeight = sequence->bpmPreview->enabled ? 70 : 60;

	panelContainer.setBounds(r.removeFromLeft(panelWidth));
	grabber.setBounds(r.removeFromLeft(6));
	timelineContainer.setBounds(r);

	Rectangle<int> panelR = panelContainer.getLocalBounds();
	Rectangle<int> timelineR = timelineContainer.getLocalBounds();

	transportUI->setBounds(panelR.removeFromTop(headerHeight));
	navigationUI->setBounds(timelineR.removeFromTop(headerHeight));

	panelManagerUI.setBounds(panelR);
	timelineManagerUI.setBounds(timelineR);
}

void SequenceEditorView::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double /*newRangeStart*/)
{
	if (scrollBarThatHasMoved == &panelManagerUI.viewport.getVerticalScrollBar())
	{
		timelineManagerUI.viewport.setViewPosition(panelManagerUI.viewport.getViewPosition());
	}
	else
	{
		panelManagerUI.viewport.setViewPosition(timelineManagerUI.viewport.getViewPosition());
	}
}

void SequenceEditorView::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& details)
{
	//DBG("Mouse wheel move " << (int)panelManagerUI.isMouseOver(true) << ", " << (int)timelineManagerUI.isMouseOver(true) << " / deltaX : " << details.deltaX << ", deltaY : " << details.deltaY);	

	if (details.deltaY != 0 && !e.mods.isCommandDown())
	{
		if (e.mods.isShiftDown() || e.originalComponent == navigationUI->seeker.get() || e.originalComponent == &navigationUI->seeker->handle)
		{
			//float sequenceViewMid = (sequence->viewStartTime->floatValue() + sequence->viewEndTime->floatValue()) / 2;	
			//float zoomFactor = details.deltaY; //*navigationUI.seeker.getTimeForX(details.deltaY);	
			float initDist = sequence->viewEndTime->floatValue() - sequence->viewStartTime->floatValue();
			float zoomFactor = (details.deltaY * initDist);

			float time = navigationUI->header->getTimeForX(e.getEventRelativeTo(navigationUI->header.get()).x);

			float viewStart = sequence->viewStartTime->floatValue();
			float viewRange = sequence->viewEndTime->floatValue() - viewStart;

			float diff = (time - viewStart) / viewRange;

			float newViewStart = sequence->viewStartTime->floatValue() + zoomFactor * diff;
			float newViewEnd = sequence->viewEndTime->floatValue() - zoomFactor * (1 - diff);
			float newViewRange = newViewEnd - newViewStart;

			if (newViewRange >= sequence->minViewTime->floatValue())
			{
				bool followPlayingMode = sequence->isPlaying->boolValue() && sequence->viewFollowTime->boolValue();
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
			if (panelManagerUI.getLocalBounds().contains(panelManagerUI.getMouseXYRelative())) //hack, need to ask Jules about listenedComponent for direct listener to event information, also have a unique "scrollbar" event for wheel+drag	
			{
				timelineManagerUI.viewport.setViewPosition(panelManagerUI.viewport.getViewPosition());
			}
			else if (timelineManagerUI.getLocalBounds().contains(timelineManagerUI.getMouseXYRelative()))
			{
				panelManagerUI.viewport.setViewPosition(timelineManagerUI.viewport.getViewPosition());
			}
		}
	}

	if (details.deltaX != 0 || (details.deltaY != 0 && e.mods.isCommandDown()))
	{
		float wheelVal = details.deltaX == 0 ? details.deltaY : details.deltaX;
		float initDist = sequence->viewEndTime->floatValue() - sequence->viewStartTime->floatValue();
		sequence->viewStartTime->setValue(jmin(sequence->viewStartTime->floatValue() - initDist * wheelVal, sequence->totalTime->floatValue() - initDist));
		sequence->viewEndTime->setValue(sequence->viewStartTime->floatValue() + initDist);
	}
}

void SequenceEditorView::mouseMagnify(const MouseEvent& e, float scaleFactor)
{
	float initDist = sequence->viewEndTime->floatValue() - sequence->viewStartTime->floatValue();
	float zoomFactor = (scaleFactor * initDist) / 2;
	sequence->viewStartTime->setValue(sequence->viewStartTime->floatValue() + zoomFactor);
	sequence->viewEndTime->setValue(sequence->viewEndTime->floatValue() - zoomFactor);
}

bool SequenceEditorView::keyPressed(const KeyPress& key)
{
	if (sequence == nullptr) return false;

	if (key.getKeyCode() == KeyPress::homeKey)
	{
		sequence->currentTime->setValue(0);
	}
	else if (key.getKeyCode() == KeyPress::endKey)
	{
		sequence->currentTime->setValue(sequence->totalTime->floatValue());
	}


	return false;
}

void SequenceEditorView::seekerManipulationChanged(bool isManipulating)
{
	for (auto& layerUI : timelineManagerUI.itemsUI)
	{
		layerUI->setSeekManipulationMode(isManipulating);
	}
}

void SequenceEditorView::newMessage(const ContainerAsyncEvent& e)
{
	if (e.type == e.ControllableStateUpdate)
	{
		if (e.targetControllable == sequence->bpmPreview)
		{
			resized();
		}
	}
}

void SequenceEditorView::grabberGrabUpdate(GapGrabber*, int relativeDist)
{
	panelWidth += relativeDist;
	resized();
}

void SequenceEditorView::grabberGrabEvent(GapGrabber*, bool isGrabbing)
{
	for (auto& layerUI : timelineManagerUI.itemsUI)
	{
		layerUI->setSeekManipulationMode(isGrabbing);
	}
}
