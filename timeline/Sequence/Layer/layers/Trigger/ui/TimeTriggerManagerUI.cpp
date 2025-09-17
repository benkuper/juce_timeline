/*
  ==============================================================================

    TimeTriggerManagerUI.cpp
    Created: 10 Dec 2016 12:23:33pm
    Author:  Ben

  ==============================================================================
*/

TimeTriggerManagerUI::TimeTriggerManagerUI(TriggerLayerTimeline * _timeline, TimeTriggerManager * manager) :
	ManagerUI("Triggers", manager, false),
	timeline(_timeline),
	miniMode(false)
{
	addItemText = "Add Trigger";
	animateItemOnAdd = false;
	transparentBG = true;

	addItemBT->setVisible(false);

	noItemText = "To add triggers, double click here";

	manager->selectionManager->addSelectionListener(this);

	resizeOnChildBoundsChanged = false;
	addExistingItems();
}

TimeTriggerManagerUI::~TimeTriggerManagerUI()
{
	manager->selectionManager->removeSelectionListener(this);
	if (InspectableSelector::getInstanceWithoutCreating()) InspectableSelector::getInstance()->removeSelectorListener(this);
}

void TimeTriggerManagerUI::setMiniMode(bool value)
{
	if (miniMode == value) return;
	miniMode = value;

	for (auto& i : itemsUI) i->setInterceptsMouseClicks(!miniMode, !miniMode);
}

void TimeTriggerManagerUI::resized()
{
	updateContent();
}

void TimeTriggerManagerUI::updateContent()
{
	for (auto &ttui : itemsUI)
	{
		placeTimeTriggerUI(ttui);
		if(ttui->item->isSelected) ttui->toFront(true);
		else ttui->toBack();
	}
}

void TimeTriggerManagerUI::placeTimeTriggerUI(TimeTriggerUI * ttui)
{
	int tx = timeline->getXForTime(ttui->item->time->floatValue());
	float totalTime = timeline->item->sequence->totalTime->floatValue();

	float maxX = timeline->getXForTime(totalTime);

	int triggerWidth = 0;
	float length = ttui->item->length->floatValue();
	if(length > 0.f)
	{
		triggerWidth = timeline->getXForTime(ttui->item->time->floatValue() + length) - tx;
	}

	int endX = tx + ttui->labelWidth;

	if (endX > maxX && tx <= maxX)
	{
		int diff = ttui->labelWidth - 1;

		tx -= diff;
		ttui->startXOffset = diff;
		ttui->flagXOffset = 0;
	}
	else
	{
		ttui->startXOffset = 0;
		ttui->flagXOffset = 0;
	}
	ttui->triggerWidth = triggerWidth;

	ttui->setBounds(tx, 0, ttui->labelWidth + triggerWidth + 1, getHeight());
}

void TimeTriggerManagerUI::mouseDown(const MouseEvent & e)
{
	ManagerUI::mouseDown(e);

	if (e.eventComponent == this)
	{
		if (e.mods.isLeftButtonDown())
		{
			if (e.mods.isAltDown())
			{
				if (manager->managerFactory == nullptr || manager->managerFactory->defs.size() == 1)
				{
					float time = timeline->getTimeForX(getMouseXYRelative().x);
					manager->addTriggerAt(time, getMouseXYRelative().y * 1.f / getHeight());
				}
			}
		}
	}
}

void TimeTriggerManagerUI::mouseDoubleClick(const MouseEvent & e)
{
	if (miniMode) return;
	if (manager->managerFactory != nullptr && manager->managerFactory->defs.size() > 1) return;

	float time = timeline->getTimeForX(getMouseXYRelative().x);
	manager->addTriggerAt(time, getMouseXYRelative().y*1.f / getHeight());
}

void TimeTriggerManagerUI::addItemFromMenu(bool isFromAddButton, Point<int> mouseDownPos)
{
	if (isFromAddButton || miniMode) return;

	float time = timeline->getTimeForX(mouseDownPos.x);
	manager->addTriggerAt(time, mouseDownPos.y*1.f / getHeight());
}

void TimeTriggerManagerUI::addItemFromMenu(TimeTrigger* t, bool, Point<int> mouseDownPos)
{
	float time = timeline->getTimeForX(mouseDownPos.x);
	t->time->setValue(time);
	t->flagY->setValue(mouseDownPos.y * 1.f / getHeight());
	manager->addItem(t);
}

void TimeTriggerManagerUI::addBaseItemUIInternal(TimeTriggerUI * ttui)
{
	ttui->addTriggerUIListener(this);
}

void TimeTriggerManagerUI::removeBaseItemUIInternal(TimeTriggerUI * ttui)
{
	ttui->removeTriggerUIListener(this);
}

void TimeTriggerManagerUI::timeTriggerMouseDown(TimeTriggerUI* ttui, const MouseEvent& e)
{
	snapTimes.clear();
	if (getSnapTimesFunc != nullptr) getSnapTimesFunc(&snapTimes);
}

void TimeTriggerManagerUI::timeTriggerDragged(TimeTriggerUI * ttui, const MouseEvent & e)
{
	if (miniMode) return;

	float diffTime = timeline->getTimeForX(e.getOffsetFromDragStart().x, false);
	//if (e.mods.isShiftDown()) diffTime = timeline->item->sequence->cueManager->getNearestCueForTime(ttui->timeAtMouseDown + diffTime) - ttui->timeAtMouseDown;

	float length = ttui->lengthAtMouseDown;
	Point<float> offset(0.f, 0.f);

	if (e.mods.isAltDown() && manager->selectionManager->currentInspectables.size() >= 2)
	{
		ttui->item->scalePosition(Point<float>(diffTime, 0), true);
		return;
	}
	else if(ttui->draggingLength)
	{
		if (e.mods.isShiftDown() || timeline->item->sequence->autoSnap->boolValue())
		{
			float targetTime = timeline->item->sequence->getClosestSnapTimeFor(snapTimes, ttui->item->movePositionReference.x + length + diffTime) - ttui->item->movePositionReference.x;
			if(e.mods.isCtrlDown())
			{
				offset.x = targetTime - length;
			}
			else
			{
				length = targetTime;
			}
		}
		else if(e.mods.isCtrlDown())
		{
			offset.x = diffTime;
		}
		else
		{
			length += diffTime;
		}
	}
	else
	{
		offset.y = (e.mods.isAltDown() || e.mods.isCtrlDown())?0:e.getDistanceFromDragStartY()*1.0f / (getHeight() -20); //-20 is for subtracting flag height

		if (e.mods.isShiftDown() || timeline->item->sequence->autoSnap->boolValue())
		{
			offset.x = timeline->item->sequence->getClosestSnapTimeFor(snapTimes, ttui->item->movePositionReference.x + diffTime) - ttui->item->movePositionReference.x;
		}
		else
		{
			offset.x = diffTime;
		}

		if(e.mods.isCtrlDown())
		{
			length -= offset.x;
		}
	}

	ttui->item->movePosition(offset, true);
	ttui->item->length->setValue(length);
}

void TimeTriggerManagerUI::timeTriggerTimeChanged(TimeTriggerUI * ttui)
{
	placeTimeTriggerUI(ttui);
}

void TimeTriggerManagerUI::timeTriggerDeselected(TimeTriggerUI* ttui)
{
	placeTimeTriggerUI(ttui);
}

void TimeTriggerManagerUI::selectionEnded(Array<Component*> selectedComponents)
{
	if(InspectableSelector::getInstanceWithoutCreating()) InspectableSelector::getInstance()->removeSelectorListener(this);
	if (selectedComponents.size() == 0) timeline->item->selectThis();
}
