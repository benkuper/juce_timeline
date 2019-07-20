#include "TimedLayerItem.h"

TimedLayerItem::TimedLayerItem() :
	moveTimeReference(0)
{
}

void TimedLayerItem::setMoveTimeReference(bool setOtherSelectedItems)
{
	setMoveTimeReferenceInternal();

	if (setOtherSelectedItems)
	{
		Array<TimedLayerItem*> items = InspectableSelectionManager::activeSelectionManager->getInspectablesAs<TimedLayerItem>();
		for (auto& i : items)
		{
			if (i == this) continue;
			i->setMoveTimeReference(false);
		}
	}
}

void TimedLayerItem::moveTime(float timeOffset, bool moveOtherSelectedItems)
{
	setTime(moveTimeReference + timeOffset);
	if (moveOtherSelectedItems)
	{
		Array<TimedLayerItem *> items = InspectableSelectionManager::activeSelectionManager->getInspectablesAs<TimedLayerItem>();
		for (auto& i : items)
		{
			if (i == this) continue;
			i->moveTime(timeOffset, false);
		}
	}
}

void TimedLayerItem::addMoveToUndoManager(bool addOtherSelectedItems)
{
	Array<UndoableAction*> actions;
	
	if (!addOtherSelectedItems)
	{
		UndoMaster::getInstance()->performAction("Move item",getUndoableMoveAction());
	}
	else
	{
		Array<TimedLayerItem*> items = InspectableSelectionManager::activeSelectionManager->getInspectablesAs<TimedLayerItem>();
		for (auto& i : items)
		{
			actions.add(i->getUndoableMoveAction());
		}

		UndoMaster::getInstance()->performActions("Move " + String(items.size()) + " items", actions);
	}
	
}
