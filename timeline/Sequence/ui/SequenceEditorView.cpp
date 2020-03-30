/*
  ==============================================================================

    SequenceEditor.cpp
    Created: 28 Oct 2016 8:27:18pm
    Author:  bkupe

  ==============================================================================
*/

SequenceEditorView::SequenceEditorView(Sequence * _sequence, SequenceTimelineNavigationUI * navigationUI, SequenceTransportUI * transportUI) :
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

	addAndMakeVisible(&grabber);
	grabber.addGrabberListener(this);

	
}

SequenceEditorView::~SequenceEditorView()
{
	if (!sequenceRef.wasObjectDeleted()) sequence->setBeingEdited(false); 
}

void SequenceEditorView::paint(Graphics &)
{
	//g.fillAll(Colours::red);
	//g.setColour(Colours::yellow);
	//g.drawLine(panelWidth + 1, 0, panelWidth + 1, getHeight(), 1);
}

void SequenceEditorView::resized()
{
	Rectangle<int> r = getLocalBounds();

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

	if (details.deltaY != 0)
	{
		if (e.mods.isShiftDown() || e.originalComponent == navigationUI->seeker.get() || e.originalComponent == &navigationUI->seeker->handle)
		{
			//float sequenceViewMid = (sequence->viewStartTime->floatValue() + sequence->viewEndTime->floatValue()) / 2;	
			//float zoomFactor = details.deltaY; //*navigationUI.seeker.getTimeForX(details.deltaY);	
			float initDist = sequence->viewEndTime->floatValue() - sequence->viewStartTime->floatValue();
			float zoomFactor = (details.deltaY * initDist) / 2;
			sequence->viewStartTime->setValue(sequence->viewStartTime->floatValue() + zoomFactor);
			sequence->viewEndTime->setValue(sequence->viewEndTime->floatValue() - zoomFactor);

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

	if (details.deltaX != 0)
	{
		float initDist = sequence->viewEndTime->floatValue() - sequence->viewStartTime->floatValue();
		sequence->viewStartTime->setValue(jmin(sequence->viewStartTime->floatValue() - initDist * details.deltaX, sequence->totalTime->floatValue() - initDist));
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

void SequenceEditorView::grabberGrabUpdate(GapGrabber *, int relativeDist)
{
	panelWidth += relativeDist;
	resized();
}
