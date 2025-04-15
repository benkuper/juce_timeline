/*
  ==============================================================================

	TimeTriggerManager.cpp
	Created: 10 Dec 2016 12:22:48pm
	Author:  Ben

  ==============================================================================
*/

#include "JuceHeader.h"

TimeTriggerManager::TimeTriggerManager(TriggerLayer* _layer, Sequence* _sequence) :
	Manager("Triggers"),
	layer(_layer),
	sequence(_sequence)
{
	hideInEditor = true;

	comparator.compareFunc = &TimeTriggerManager::compareTime;

	itemDataType = "TimeTrigger";

	sequence->addSequenceListener(this);

}

TimeTriggerManager::~TimeTriggerManager()
{
	if (!sequence->isClearing)
		sequence->removeSequenceListener(this);
}


void TimeTriggerManager::addTriggerAt(float time, float flagY)
{
	TimeTrigger* t = createItem();
	t->time->setValue(time);
	t->time->resetLastUndoValue();
	t->flagY->setValue(flagY);
	t->flagY->resetLastUndoValue();
	Manager::addItem(t);
}

void TimeTriggerManager::addItemInternal(TimeTrigger* t, var data)
{
	t->time->setRange(0, sequence->totalTime->floatValue());
}

void TimeTriggerManager::addItemsInternal(Array<TimeTrigger*> items, var data)
{
	for (auto& t : items) t->time->setRange(0, sequence->totalTime->floatValue());
}

Array<TimeTrigger*> TimeTriggerManager::addItemsFromClipboard(bool showWarning)
{
	Array<TimeTrigger*> triggers = Manager::addItemsFromClipboard(showWarning);
	if (triggers.isEmpty()) return triggers;
	if (triggers[0] == nullptr) return Array<TimeTrigger*>();

	float minTime = triggers[0]->time->floatValue();
	for (auto& tt : triggers)
	{
		if (tt->time->floatValue() < minTime)
		{
			minTime = tt->time->floatValue();
		}
	}

	float diffTime = sequence->currentTime->floatValue() - minTime;
	for (auto& tt : triggers) tt->time->setValue(tt->time->floatValue() + diffTime);

	reorderItems();

	return triggers;
}

bool TimeTriggerManager::canAddItemOfType(const String& typeToCheck)
{
	return typeToCheck == itemDataType || typeToCheck == "Action";
}

TimeTrigger* TimeTriggerManager::getPrevTrigger(float time, bool includeCurrentTime)
{
	for (int i = items.size() - 1; i >= 0; i--)
	{
		TimeTrigger* tt = items[i];
		if (tt->time->floatValue() < time || (tt->time->floatValue() == time && includeCurrentTime)) return tt;
	}
	return nullptr;
}

TimeTrigger* TimeTriggerManager::getNextTrigger(float time, bool includeCurrentTime)
{
	for (auto& tt : items)
	{
		if (tt->time->floatValue() > time || (tt->time->floatValue() == time && includeCurrentTime)) return tt;
	}
	return nullptr;
}

Array<TimeTrigger*> TimeTriggerManager::getTriggersInTimespan(float startTime, float endTime, bool includeAlreadyTriggered)
{
	Array<TimeTrigger*> result;
	for (auto& tt : items)
	{
		if (tt->time->floatValue() >= startTime && tt->time->floatValue() <= endTime && (includeAlreadyTriggered || !tt->isTriggered->boolValue()))
		{
			result.add(tt);
		}
	}
	return result;
}

Array<UndoableAction*> TimeTriggerManager::getMoveKeysBy(float start, float offset)
{
	Array<UndoableAction*> actions;
	Array<TimeTrigger*> triggers = getTriggersInTimespan(start, sequence->totalTime->floatValue());
	for (auto& t : triggers) actions.addArray(t->time->setUndoableValue(t->time->floatValue() + offset, true, false));
	return actions;
}

Array<UndoableAction*> TimeTriggerManager::getRemoveTimespan(float start, float end)
{
	Array<UndoableAction*> actions;
	Array<TimeTrigger*> triggers = getTriggersInTimespan(start, end);
	actions.addArray(getRemoveItemsUndoableAction(triggers));
	actions.addArray(getMoveKeysBy(end, start - end));
	return actions;
}

void TimeTriggerManager::reorderActionsMap()
{
	actionsMap.clear();

	auto endIt = actionsMap.end();

	for (auto& tt : items)
	{
		float triggerStart = tt->time->floatValue();

		actionsMap.emplace_hint(endIt, triggerStart, std::make_pair(tt, true)); // automatically sorted

		if(tt->length->floatValue() <= 0.f) continue;

		float triggerEnd = triggerStart + tt->length->floatValue();
		actionsMap.emplace_hint(endIt, triggerEnd, std::make_pair(tt, false));
	}
}

void TimeTriggerManager::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	TimeTrigger* t = static_cast<TimeTrigger*>(cc);
	if (t != nullptr)
	{
		if (c == t->time)
		{
			int index = items.indexOf(t);
			if (index > 0 && t->time->floatValue() < items[index - 1]->time->floatValue())
			{
				items.swap(index, index - 1);
				managerListeners.call(&ManagerListener::itemsReordered);
			}
			else if (index < items.size() - 1 && t->time->floatValue() > items[index + 1]->time->floatValue())
			{
				items.swap(index, index + 1);
				managerListeners.call(&ManagerListener::itemsReordered);
			}

			reorderActionsMap();
		}
		else if (c == t->canTrigger || c == enabled)
		{
			sequencePlayStateChanged(nullptr);
		}

	}
}

void TimeTriggerManager::reorderItems()
{
	Manager<TimeTrigger>::reorderItems();
	reorderActionsMap();
}

void TimeTriggerManager::executeTriggersTimespan(float startTime, float endTime, bool forward, bool onlyUntrigger)
{
	if(forward)
	{
		for (auto item = actionsMap.begin(); item != actionsMap.end() ; item++)
		{
			if(item->first > endTime) break;
			if(item->first < startTime) continue;

			if (!onlyUntrigger || !item->second.second)
				item->second.first->setTriggerState(item->second.second, onlyUntrigger);
		}
	}
	else
	{
		for (auto item = actionsMap.rbegin(); item != actionsMap.rend() ; item++)
		{
			if(item->first < startTime) break;
			if(item->first > endTime) continue;

			if (!onlyUntrigger || item->second.second)
				item->second.first->setTriggerState(!item->second.second, onlyUntrigger);
		}
	}
}

void TimeTriggerManager::sequenceCurrentTimeChanged(Sequence* /*_sequence*/, float prevTime, bool evaluateSkippedData)
{
	if (!layer->enabled->boolValue() || !sequence->enabled->boolValue()) return;

	float curTime = sequence->currentTime->floatValue();
	if (curTime == prevTime) return;

	bool playingForward = sequence->playSpeed->floatValue() >= 0;
	bool diffIsForward = curTime >= prevTime;
	bool normallyPlaying = playingForward == diffIsForward;

	float minTime = jmin(prevTime, curTime);
	float maxTime = jmax(prevTime, curTime);

	if (normallyPlaying)
	{
		if ((sequence->isPlaying->boolValue() && !sequence->isSeeking) || evaluateSkippedData || ModifierKeys::getCurrentModifiers().isCtrlDown())
		{
			executeTriggersTimespan(minTime, maxTime, playingForward);
		}
	}
	else //loop or manual, untrigger
	{
		executeTriggersTimespan(minTime, maxTime, !playingForward, true);
		sequencePlayDirectionChanged(nullptr);
	}
}

void TimeTriggerManager::sequencePlayStateChanged(Sequence*)
{
	if(sequence->isPlaying->boolValue())
	{
		float curTime = sequence->currentTime->floatValue();

		for (auto& tt : items)
		{
			if(tt->length->floatValue() <= 0.f) continue;

			float triggerStart = tt->time->floatValue();
			float triggerEnd = triggerStart + tt->length->floatValue();

			if(curTime >= triggerStart && curTime <= triggerEnd)
			{
				tt->updateTriggerState();
			}
		}
	}
}

void TimeTriggerManager::sequenceTotalTimeChanged(Sequence*)
{
	for (auto& t : items) t->time->setRange(0, sequence->totalTime->floatValue());
}

void TimeTriggerManager::sequencePlayDirectionChanged(Sequence*)
{
	for (auto& i : items) i->isTriggered->setValue(false);
}

void TimeTriggerManager::sequenceLooped(Sequence*)
{
	for (auto& t : items) t->isTriggered->setValue(false);
}

int TimeTriggerManager::compareTime(TimeTrigger* t1, TimeTrigger* t2)
{
	if (t1->time->floatValue() < t2->time->floatValue()) return -1;
	else if (t1->time->floatValue() > t2->time->floatValue()) return 1;
	return 0;
}
