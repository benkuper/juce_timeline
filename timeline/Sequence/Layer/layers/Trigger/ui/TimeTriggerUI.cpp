/*
  ==============================================================================

	TimeTriggerUI.cpp
	Created: 10 Dec 2016 11:57:16am
	Author:  Ben

  ==============================================================================
*/

TimeTriggerUI::TimeTriggerUI(TimeTrigger * _tt) :
	BaseItemUI<TimeTrigger>(_tt, Direction::NONE),
	labelWidth(0),
	triggerWidth(0),
	startXOffset(0),
	flagXOffset(0)
{
	dragAndDropEnabled = false; //avoid default behavior

	autoDrawContourWhenSelected = false;
	setName(_tt->niceName);

	lockUI.reset(item->isUILocked->createToggle(ImageCache::getFromMemory(OrganicUIBinaryData::padlock_png, OrganicUIBinaryData::padlock_pngSize)));
	addAndMakeVisible(lockUI.get());

	removeBT->setVisible(item->isSelected);
	enabledBT->setVisible(item->isSelected);
	lockUI->setVisible(item->isSelected);
	itemColorUI->setVisible(item->isSelected);

	updateSizeFromName();

}

TimeTriggerUI::~TimeTriggerUI()
{
}

void TimeTriggerUI::paint(Graphics & g)
{
	Colour c = item->itemColor->getColor();
	if (!item->enabled->boolValue()) c = c.darker(.6f).withAlpha(.7f);

	g.setColour(c);
	
	g.fillRect(flagRect);


	if (item->isUILocked->boolValue())
	{
		g.setTiledImageFill(ImageCache::getFromMemory(TimelineBinaryData::smallstripe_png, TimelineBinaryData::smallstripe_pngSize), 0, 0, .1f); 
		g.fillRect(flagRect);
	}

	if(item->isSelected)
	{
		c = HIGHLIGHT_COLOR;
	}
	else if(item->isPreselected)
	{
		c = PRESELECT_COLOR;
	}
	else if (item->isTriggered->boolValue())
	{
		c = GREEN_COLOR.darker();
	}

	g.setColour(c.brighter());
	g.drawRect(flagRect);
	g.drawVerticalLine(startXOffset, 0, (float)getHeight());

	if(triggerWidth > 0)
	{
		g.drawVerticalLine(startXOffset + triggerWidth, 0, (float)getHeight());
		g.setColour(c.interpolatedWith(item->itemColor->getColor(), 0.7f).withAlpha(.4f));
		g.fillRect(startXOffset + 1, 0, triggerWidth - 1, getHeight());
	}

}

void TimeTriggerUI::resized()
{
	Rectangle<int> r = getLocalBounds();

	int ty = (int)(item->flagY->floatValue()*(getHeight() - 20));

	flagRect = r.translated(flagXOffset, ty).withSize(labelWidth, 20);
	lineRect = r.translated(startXOffset, 0).withWidth(6);
	if(labelWidth > 0)
		lengthRect = r.translated(startXOffset + triggerWidth - 6, 0).withWidth(6);

	Rectangle<int> p = flagRect.reduced(2, 2);
	if (item->isSelected)
	{

		removeBT->setBounds(p.removeFromRight(p.getHeight()));
		p.removeFromRight(2);
		itemColorUI->setBounds(p.removeFromRight(p.getHeight()).reduced(1));
		enabledBT->setBounds(p.removeFromRight(15));
		p.removeFromRight(2);
		if(lockUI != nullptr) lockUI->setBounds(p.removeFromRight(p.getHeight()));
	}

	itemLabel.setBounds(p);
}

bool TimeTriggerUI::hitTest(int x, int y)
{
	if (flagRect.contains(x, y)) return true;
	if (lineRect.contains(x, y)) return true;
	if (labelWidth > 0 && lengthRect.contains(x, y)) return true;
	return false;
}

void TimeTriggerUI::updateSizeFromName()
{
	int newWidth = TextLayout::getStringWidth(itemLabel.getFont(), itemLabel.getText())+ 15;
	if (item->isSelected)
	{
		newWidth += 60; //for all the buttons
	}
	labelWidth = newWidth;
}

void TimeTriggerUI::mouseDown(const MouseEvent& e)
{
	BaseItemUI::mouseDown(e);

	flagYAtMouseDown = item->flagY->floatValue();

	if (item->isUILocked->boolValue()) return;

	float lengthDragPos = startXOffset + triggerWidth;
	float xCursorPos = e.getPosition().getX();

	if(labelWidth > 0 && lengthRect.contains(e.getPosition()))
	{
		draggingLength = true;
	}
	else
	{
		item->setMovePositionReference(true);
		draggingLength = false;
	}

	lengthAtMouseDown = item->length->floatValue();
	timeAtMouseDown = item->time->floatValue();
	triggerUIListeners.call(&TimeTriggerUIListener::timeTriggerMouseDown, this, e);
}

void TimeTriggerUI::mouseDrag(const MouseEvent & e)
{
	if (itemLabel.isBeingEdited()) return;

	BaseItemUI::mouseDrag(e);
	
	if (item->isUILocked->boolValue()) return; //After that, nothing will changed if item is locked
	
	if (e.mods.isLeftButtonDown())
	{
		if(draggingLength != e.mods.isCtrlDown())
		{
			setMouseCursor(MouseCursor::LeftRightResizeCursor);
		}
		else
		{
			setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
		}
	
		updateMouseCursor();

		triggerUIListeners.call(&TimeTriggerUIListener::timeTriggerDragged, this, e);
	}

/*	if (!e.mods.isCommandDown() && item->selectionManager->currentInspectables.size() == 1)
	{
		float ty = flagYAtMouseDown + e.getOffsetFromDragStart().y * 1.f / (getHeight() - 20);
		item->flagY->setValue(ty);
	}
	*/
}

void TimeTriggerUI::mouseUp(const MouseEvent & e)
{
	BaseItemUI::mouseUp(e);

	Array<UndoableAction*> actions;

	if(lengthAtMouseDown != item->length->floatValue())
	{
		actions.addArray(item->length->setUndoableValue(lengthAtMouseDown, item->length->floatValue(), true, false));
	}

	if (flagYAtMouseDown != item->flagY->floatValue())
	{
		actions.addArray(item->flagY->setUndoableValue(flagYAtMouseDown, item->flagY->floatValue(), true, false));
	}
	
	if(timeAtMouseDown != item->time->floatValue())
	{
		if (item->selectionManager->currentInspectables.size() >= 2)
		{
			item->addMoveToUndoManager(true);
		}
		else
		{
			if (!item->isUILocked->boolValue()) actions.addArray(item->time->setUndoableValue(timeAtMouseDown, item->time->floatValue(), true, false));
		}
	}

	if(!actions.isEmpty())
		UndoMaster::getInstance()->performActions("Move Trigger \"" + item->niceName + "\"", actions);
}

void TimeTriggerUI::mouseMove(const MouseEvent & e)
{
	if(item->isUILocked->boolValue())
	{
		setMouseCursor(MouseCursor::NormalCursor);
	}
	else if((labelWidth > 0 && lengthRect.contains(e.getPosition())) != e.mods.isCtrlDown())
	{
		setMouseCursor(MouseCursor::LeftRightResizeCursor);
	}
	else
	{
		setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
	}

	updateMouseCursor();
}

void TimeTriggerUI::containerChildAddressChangedAsync(ControllableContainer * cc)
{
	BaseItemUI::containerChildAddressChangedAsync(cc);
	updateSizeFromName();
}

void TimeTriggerUI::controllableFeedbackUpdateInternal(Controllable * c)
{
	if (c == item->time || c == item->length)
	{
		triggerUIListeners.call(&TimeTriggerUIListener::timeTriggerTimeChanged, this);
	} else if (c == item->flagY)
	{
		repaint();
		resized();
	} else if (c == item->isTriggered)
	{
		repaint();
	} else if (c == item->isUILocked)
	{
		repaint();
	}
}

void TimeTriggerUI::inspectableSelectionChanged(Inspectable * i)
{
	BaseItemUI::inspectableSelectionChanged(i);
	removeBT->setVisible(item->isSelected);
	enabledBT->setVisible(item->isSelected);
	if(lockUI != nullptr) lockUI->setVisible(item->isSelected);
	if(itemColorUI != nullptr) itemColorUI->setVisible(item->isSelected);

	updateSizeFromName();

	triggerUIListeners.call(&TimeTriggerUIListener::timeTriggerDeselected, this);
}
