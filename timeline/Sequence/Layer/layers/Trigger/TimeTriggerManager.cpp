/*
  ==============================================================================

	TimeTriggerManager.cpp
	Created: 10 Dec 2016 12:22:48pm
	Author:  Ben

  ==============================================================================
*/

#include "JuceHeader.h"

TimeTriggerManager::TimeTriggerManager(TriggerLayer* _layer, Sequence* _sequence) :
	BaseManager("Triggers"),
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
	t->flagY->setValue(flagY);
	BaseManager::addItem(t);
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
	Array<TimeTrigger*> triggers = BaseManager::addItemsFromClipboard(showWarning);
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
				baseManagerListeners.call(&ManagerListener::itemsReordered);
			}
			else if (index < items.size() - 1 && t->time->floatValue() > items[index + 1]->time->floatValue())
			{
				items.swap(index, index + 1);
				baseManagerListeners.call(&ManagerListener::itemsReordered);
			}
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

	if (normallyPlaying)
	{
		if ((sequence->isPlaying->boolValue() && !sequence->isSeeking) || evaluateSkippedData || ModifierKeys::getCurrentModifiers().isCtrlDown())
		{
			float minTime = jmin(prevTime, curTime);
			float maxTime = jmax(prevTime, curTime);

			Array<TimeTrigger*> spanTriggers = getTriggersInTimespan(minTime, maxTime);
			for (auto& tt : spanTriggers)
			{
				tt->trigger();
			}
		}
	}
	else //loop or manual, untrigger
	{

		float minTime = playingForward ? curTime : 0;
		float maxTime = playingForward ? sequence->totalTime->floatValue() : curTime;

		Array<TimeTrigger*> spanTriggers = getTriggersInTimespan(minTime, maxTime, true);
		for (auto& tt : spanTriggers)
		{
			tt->isTriggered->setValue(false);
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
