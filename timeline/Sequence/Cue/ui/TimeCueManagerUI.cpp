/*
  ==============================================================================

	TimeCueManagerUI.cpp
	Created: 6 Apr 2017 11:35:25am
	Author:  Ben

  ==============================================================================
*/

TimeCueManagerUI::TimeCueManagerUI(SequenceTimelineHeader* _header, TimeCueManager* manager) :
	ManagerUI("Cues", manager, false),
	header(_header)
{
	addItemText = "Add Cue";
	animateItemOnAdd = false;
	transparentBG = true;

	addItemBT->setVisible(false);
	addExistingItems();
}

TimeCueManagerUI::~TimeCueManagerUI()
{
}

void TimeCueManagerUI::paint(Graphics& g)
{
	for (auto& cui : itemsUI)
	{
		if (cui->item->cueAction->getValueDataAsEnum<TimeCue::CueAction>() == TimeCue::LOOP_JUMP)
		{
			if (TimeCue* tc = dynamic_cast<TimeCue*>(cui->item->loopCue->targetContainer.get()))
			{
				int x1 = header->getXForTime(tc->time->floatValue());
				int x2 = header->getXForTime(cui->item->time->floatValue());
				int minX = jmax(jmin(x1, x2), 0);
				int maxX = jmin(jmax(x1, x2), getWidth());

				g.setColour((x1 > x2 ? YELLOW_COLOR : RED_COLOR).withAlpha(.4f));
				g.fillRect(Rectangle<int>(minX, 2, maxX - minX, getHeight() - 2));
			}
		}
	}
}

void TimeCueManagerUI::resized()
{
	updateContent();
}

void TimeCueManagerUI::updateContent()
{
	for (auto& tui : itemsUI)
	{
		//DBG("place " << tui->item->time->floatValue());
		placeTimeCueUI(tui);
		if (tui->item->isSelected) tui->toFront(true);
		else tui->toBack();
	}
}

bool TimeCueManagerUI::hitTest(int x, int y)
{

	for (auto& i : itemsUI)
	{
		if (i->getBounds().contains(Point<int>(x, y)))
		{
			return true;
		}
	}

	return false;
}

void TimeCueManagerUI::placeTimeCueUI(TimeCueUI* ttui)
{
	int tx = header->getXForTime(ttui->item->time->floatValue());
	ttui->setBounds(tx - ttui->arrowSize / 2, 0, ttui->getWidth(), getHeight());
}


void TimeCueManagerUI::addCueAtPos(int x)
{
	float time = header->getTimeForX(getMouseXYRelative().x);
	manager->addCueAt(time);
}


void TimeCueManagerUI::addItemFromMenu(bool isFromAddButton, Point<int> mouseDownPos)
{
	if (isFromAddButton) return;

	float time = header->getTimeForX(mouseDownPos.x);
	manager->addCueAt(time);
}

void TimeCueManagerUI::addBaseItemUIInternal(TimeCueUI* ttui)
{
	ttui->addCueUIListener(this);
	placeTimeCueUI(ttui);
}

void TimeCueManagerUI::removeBaseItemUIInternal(TimeCueUI* ttui)
{
	ttui->removeCueUIListener(this);
}


void TimeCueManagerUI::cueMouseDown(TimeCueUI* ttui, const MouseEvent& e)
{
	snapTimes.clear();
	header->sequence->getSnapTimes(&snapTimes);
}

void TimeCueManagerUI::cueMouseUp(TimeCueUI* ttui, const MouseEvent& e)
{
	manager->reorderItems();
}

void TimeCueManagerUI::cueDragged(TimeCueUI* ttui, const MouseEvent& e)
{
	float targetTime = header->getTimeForX(getMouseXYRelative().x);

	if (e.mods.isShiftDown() || header->sequence->autoSnap->boolValue())
	{
		targetTime = header->sequence->getClosestSnapTimeFor(snapTimes, targetTime);
	}


	ttui->item->time->setValue(targetTime);

	repaint();

}

void TimeCueManagerUI::cueTimeChanged(TimeCueUI* ttui)
{
	placeTimeCueUI(ttui);
}
