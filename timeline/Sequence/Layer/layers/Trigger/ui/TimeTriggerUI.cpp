/*
  ==============================================================================

	TimeTriggerUI.cpp
	Created: 10 Dec 2016 11:57:16am
	Author:  Ben

  ==============================================================================
*/

TimeTriggerUI::TimeTriggerUI(TimeTrigger * _tt) :
	BaseItemUI<TimeTrigger>(_tt, Direction::NONE),
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

	if (item->isTriggered->boolValue()) c = GREEN_COLOR.darker();

	g.setColour(item->isSelected?HIGHLIGHT_COLOR:item->isPreselected?PRESELECT_COLOR:c.brighter());
	g.drawRect(flagRect);
	g.drawVerticalLine(flagXOffset, 0, (float)getHeight());

}

void TimeTriggerUI::resized()
{
	Rectangle<int> r = getLocalBounds();

	int ty = (int)(item->flagY->floatValue()*(getHeight() - 20));

	flagRect = r.translated(0, ty).withHeight(20);
	lineRect = r.withWidth(6);

	Rectangle<int> p = flagRect.reduced(2, 2);
	if (item->isSelected)
	{

		removeBT->setBounds(p.removeFromRight(p.getHeight()));
		p.removeFromRight(2);
		itemColorUI->setBounds(p.removeFromRight(p.getHeight()).reduced(1));
		enabledBT->setBounds(p.removeFromRight(15));
		p.removeFromRight(2);
		lockUI->setBounds(p.removeFromRight(p.getHeight()));
	}

	itemLabel.setBounds(p);
}

bool TimeTriggerUI::hitTest(int x, int y)
{
	if (flagRect.contains(x, y)) return true;
	if (lineRect.contains(x, y)) return true;
	return false;
}

void TimeTriggerUI::updateSizeFromName()
{
	int newWidth = itemLabel.getFont().getStringWidth(itemLabel.getText())+ 15;
	if (item->isSelected)
	{
		newWidth += 60; //for all the buttons
	}
	setSize(newWidth, getHeight());
}

void TimeTriggerUI::mouseDown(const MouseEvent& e)
{
	BaseItemUI::mouseDown(e);

	flagYAtMouseDown = item->flagY->floatValue();

	if (item->isUILocked->boolValue()) return;

	item->setMovePositionReference(true);
	timeAtMouseDown = item->time->floatValue();
	posAtMouseDown = getX();

	triggerUIListeners.call(&TimeTriggerUIListener::timeTriggerMouseDown, this, e);
}

void TimeTriggerUI::mouseDrag(const MouseEvent & e)
{
	if (itemLabel.isBeingEdited()) return;

	BaseItemUI::mouseDrag(e);
	
	if (item->isUILocked->boolValue()) return; //After that, nothing will changed if item is locked
	
	if (e.mods.isLeftButtonDown())
	{
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

	if (flagYAtMouseDown == item->flagY->floatValue() && timeAtMouseDown == item->time->floatValue()) return;

	if (item->selectionManager->currentInspectables.size() >= 2)
	{
		item->addMoveToUndoManager(true);
	}
	else
	{
		Array<UndoableAction*> actions;
		actions.add(item->flagY->setUndoableValue(flagYAtMouseDown, item->flagY->floatValue(), true));
		if (!item->isUILocked->boolValue()) actions.add(item->time->setUndoableValue(timeAtMouseDown, item->time->floatValue(), true));
		UndoMaster::getInstance()->performActions("Move Trigger \"" + item->niceName + "\"", actions);
	}
}

void TimeTriggerUI::containerChildAddressChangedAsync(ControllableContainer * cc)
{
	BaseItemUI::containerChildAddressChangedAsync(cc);
	updateSizeFromName();
}

void TimeTriggerUI::controllableFeedbackUpdateInternal(Controllable * c)
{
	if (c == item->time)
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
	lockUI->setVisible(item->isSelected);
	itemColorUI->setVisible(item->isSelected);

	updateSizeFromName();
}
