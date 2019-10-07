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

void TimedLayerItem::scaleTime(float timeOffset, bool moveOtherSelectedItems)
{
	if (moveOtherSelectedItems)
	{
		Array<TimedLayerItem*> items = InspectableSelectionManager::activeSelectionManager->getInspectablesAs<TimedLayerItem>();
		
		if (items.size() < 2) return;
		TimedLayerItem* firstItem = items[0];
		TimedLayerItem* lastItem = items[0];

		float minTime = items[0]->moveTimeReference;
		float maxTime = items[0]->moveTimeReference;
		for (auto& i : items)
		{
			float t = i->moveTimeReference;
			if (t < minTime)
			{
				minTime = t;
				firstItem = i;
			}
			else if (t > maxTime)
			{
				maxTime = t;
				lastItem = i;
			}
		}

		float anchorTime = (this == firstItem) ? maxTime : minTime;

		float anchorDiffToReference = moveTimeReference - anchorTime;
		
		if (anchorDiffToReference == 0) return;

		DBG("anchor diff to selected < " << anchorDiffToReference << " / anchorTIme " << anchorTime);
		for (auto& i : items)
		{
			float  timeScale = (i->moveTimeReference - anchorTime) / anchorDiffToReference;
			float targetOffset = timeOffset * timeScale;
			DBG(" > time scale " << timeScale << " / target offset " << targetOffset);

			i->moveTime(targetOffset, false);
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
