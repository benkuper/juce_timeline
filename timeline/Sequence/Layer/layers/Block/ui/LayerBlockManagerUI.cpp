/*
  ==============================================================================

    LayerBlockManagerUI::.cpp
    Created: 14 Feb 2019 11:15:08am
    Author:  bkupe

  ==============================================================================
*/


LayerBlockManagerUI::LayerBlockManagerUI(SequenceLayerTimeline * timeline, LayerBlockManager * manager) :
	BaseManagerUI("Block Manager", manager, false),
	timeline(timeline)
{
	noItemText = "To add a block to this layer, double-click here";
	addItemText = "Add Block";
	animateItemOnAdd = false;
	transparentBG = true;

	addItemBT->setVisible(false);
}

LayerBlockManagerUI::~LayerBlockManagerUI()
{
}

void LayerBlockManagerUI::resized()
{
	updateContent();
}

void LayerBlockManagerUI::updateContent()
{
	for (auto &cui : itemsUI) placeBlockUI(cui);
}

LayerBlockUI * LayerBlockManagerUI::createUIForItem(LayerBlock * block)
{
	return new LayerBlockUI(block);
}

void LayerBlockManagerUI::placeBlockUI(LayerBlockUI * cui)
{
	int tx = timeline->getXForTime(cui->item->time->floatValue());
	int tx2 = timeline->getXForTime(cui->item->time->floatValue() + cui->item->getTotalLength());

	cui->setViewRange(timeline->item->sequence->viewStartTime->floatValue() - cui->item->time->floatValue(), timeline->item->sequence->viewEndTime->floatValue() - cui->item->time->floatValue());
	cui->setBounds(tx, 0, tx2 - tx, getHeight());
}

void LayerBlockManagerUI::mouseDoubleClick(const MouseEvent & e)
{
	if(!e.mods.isCommandDown() && !e.mods.isShiftDown() && manager->userCanAddItemsManually) manager->addBlockAt(timeline->getTimeForX(getMouseXYRelative().x));
}

void LayerBlockManagerUI::addItemFromMenu(bool isFromAddButton, Point<int> mouseDownPos)
{
	if (!manager->userCanAddItemsManually) return;
	if (isFromAddButton) return;
	manager->addBlockAt(timeline->getTimeForX(mouseDownPos.x));
}

void LayerBlockManagerUI::addItemUIInternal(LayerBlockUI * cui)
{
	cui->addBlockUIListener(this);
	placeBlockUI(cui);
}

void LayerBlockManagerUI::removeItemUIInternal(LayerBlockUI * cui)
{
	cui->removeBlockUIListener(this);
}

void LayerBlockManagerUI::blockUITimeChanged(LayerBlockUI * cui)
{
	placeBlockUI(cui);
}

void LayerBlockManagerUI::blockUIDragged(LayerBlockUI * cui, const MouseEvent & e)
{
	float targetOffsetTime = timeline->getTimeForX(e.getOffsetFromDragStart().x, false);
	//float targetTime = cui->timeAtMouseDown + targetOffsetTime;
	cui->item->moveTime(targetOffsetTime, true);
	
	cui->setViewRange(timeline->item->sequence->viewStartTime->floatValue() - cui->item->time->floatValue(), timeline->item->sequence->viewEndTime->floatValue() - cui->item->time->floatValue());
	cui->resized(); //force resize because changing time will not resize it, just move it
}

void LayerBlockManagerUI::blockUIStartDragged(LayerBlockUI * cui, const MouseEvent & e)
{
	float timeDiff = timeline->getTimeForX(e.getOffsetFromDragStart().x, false);
	float targetTime = cui->item->moveTimeReference + timeDiff;

	int itemIndex = manager->items.indexOf(cui->item);
	float minTime = (itemIndex > 0 && !manager->blocksCanOverlap) ? manager->items[itemIndex - 1]->getEndTime() : 0;
	targetTime = jmax<float>(minTime, targetTime);

	manager->placeBlockAt(cui->item, targetTime);

	float realTimeDiff = cui->item->time->floatValue() - cui->item->moveTimeReference;
	float targetCoreLength = cui->coreLengthAtMouseDown - realTimeDiff;

	cui->item->setCoreLength(targetCoreLength,e.mods.isShiftDown(), !e.mods.isAltDown());
	cui->setViewRange(timeline->item->sequence->viewStartTime->floatValue() - cui->item->time->floatValue(), timeline->item->sequence->viewEndTime->floatValue() - cui->item->time->floatValue());
}

void LayerBlockManagerUI::blockUICoreDragged(LayerBlockUI * cui, const MouseEvent & e)
{
	if (e.mods.isCommandDown())
	{
		blockUILoopDragged(cui, e);
		return;
	}

	float targetCoreLength = cui->coreLengthAtMouseDown + timeline->getTimeForX(e.getOffsetFromDragStart().x, false);

	if (!manager->blocksCanOverlap)
	{
		int itemIndex = manager->items.indexOf(cui->item);
		if (itemIndex < manager->items.size() - 1)
		{
			float maxCoreLength = manager->items[itemIndex + 1]->time->floatValue() - cui->item->loopLength->floatValue() - cui->item->time->floatValue();
			targetCoreLength = jmin<float>(maxCoreLength, targetCoreLength);
		}
	}

	cui->item->setCoreLength(targetCoreLength, e.mods.isShiftDown(), e.mods.isAltDown());
	cui->setViewRange(timeline->item->sequence->viewStartTime->floatValue() - cui->item->time->floatValue(), timeline->item->sequence->viewEndTime->floatValue() - cui->item->time->floatValue());
}

void LayerBlockManagerUI::blockUILoopDragged(LayerBlockUI * cui, const MouseEvent & e)
{
	float targetLoopLength = cui->loopLengthAtMouseDown + timeline->getTimeForX(e.getOffsetFromDragStart().x, false);
	
	if (!manager->blocksCanOverlap)
	{
		int itemIndex = manager->items.indexOf(cui->item);
		if (itemIndex < manager->items.size() - 1)
		{
			float maxLoopLength = manager->items[itemIndex + 1]->time->floatValue() - cui->item->coreLength->floatValue() - cui->item->time->floatValue();
			targetLoopLength = jmin<float>(maxLoopLength, targetLoopLength);
		}
	}
	
	
	cui->item->setLoopLength(targetLoopLength);
	cui->setViewRange(timeline->item->sequence->viewStartTime->floatValue() - cui->item->time->floatValue(), timeline->item->sequence->viewEndTime->floatValue() - cui->item->time->floatValue());
}

void LayerBlockManagerUI::blockUINeedsReorder()
{
	manager->reorderItems();
}
