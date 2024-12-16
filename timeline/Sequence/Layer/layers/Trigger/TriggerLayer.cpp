/*
  ==============================================================================

	TriggerLayer.cpp
	Created: 17 Nov 2016 7:59:54pm
	Author:  Ben Kuper

  ==============================================================================
*/

#include "JuceHeader.h"

TriggerLayer::TriggerLayer(Sequence* _sequence, StringRef name, var params) :
	SequenceLayer(_sequence, String(name)),
	ttm(nullptr)
{
	helpID = "TriggerLayer";
	//color->setColor(ACTION_COLOR.darker());

	lockAll = addTrigger("Lock All", "Lock all existing triggers in this layer");
	unlockAll = addTrigger("Unlock All", "Unlock all existing triggers in this layer");
	goToPrevKey = addTrigger("Go to Previous Key", "Go to the previous key in this layer");
	goToNextKey = addTrigger("Go to Next Key", "Go to the next key in this layer");

}

TriggerLayer::~TriggerLayer()
{
}


void TriggerLayer::setManager(TimeTriggerManager* _ttm)
{
	ttm.reset(_ttm);
	if (ttm != nullptr) addChildControllableContainer(ttm.get());
}

Array<Inspectable*> TriggerLayer::selectAllItemsBetweenInternal(float start, float end)
{
	Array<Inspectable*> result;
	result.addArray(ttm->getTriggersInTimespan(start, end, true));
	return result;
}

Array<UndoableAction*> TriggerLayer::getRemoveAllItemsBetweenInternal(float start, float end)
{
	return ttm->getRemoveItemsUndoableAction(ttm->getTriggersInTimespan(start, end, true));
}

Array<UndoableAction*> TriggerLayer::getInsertTimespanInternal(float start, float length)
{
	return ttm->getMoveKeysBy(start, length);
}

Array<UndoableAction*> TriggerLayer::getRemoveTimespanInternal(float start, float end)
{
	return ttm->getRemoveTimespan(start, end);
}

bool TriggerLayer::paste()
{
	if (ttm == nullptr) return false;
	if (!ttm->addItemsFromClipboard(false).isEmpty()) return true;
	return SequenceLayer::paste();
}

void TriggerLayer::onContainerTriggerTriggered(Trigger* t)
{
	SequenceLayer::onContainerTriggerTriggered(t);

	if (ttm == nullptr) return;

	if (t == lockAll) for (auto& i : ttm->items) i->isUILocked->setValue(true);
	else if (t == unlockAll) for (auto& i : ttm->items) i->isUILocked->setValue(false);
	else if (t == goToPrevKey)
	{
		if (TimeTrigger* tt = ttm->getPrevTrigger(sequence->currentTime->floatValue())) sequence->setCurrentTime(tt->time->floatValue());
	}
	else if (t == goToNextKey)
	{
		if (TimeTrigger* tt = ttm->getNextTrigger(sequence->currentTime->floatValue())) sequence->setCurrentTime(tt->time->floatValue());
	}
}

void TriggerLayer::getSnapTimes(Array<float>* arrayToFill)
{
	if (ttm == nullptr) return;
	for (auto& i : ttm->items) arrayToFill->addIfNotAlreadyThere(i->time->floatValue());
}

void TriggerLayer::getSequenceSnapTimesForManager(Array<float>* arrayToFill)
{
	sequence->getSnapTimes(arrayToFill);
}

var TriggerLayer::getJSONData(bool includeNonOverriden)
{
	var data = SequenceLayer::getJSONData(includeNonOverriden);
	if (ttm != nullptr) data.getDynamicObject()->setProperty(ttm->shortName, ttm->getJSONData());
	return data;
}

void TriggerLayer::loadJSONDataInternal(var data)
{
	SequenceLayer::loadJSONDataInternal(data);
	if (ttm != nullptr) ttm->loadJSONData(data.getProperty(ttm->shortName, var()));
}

SequenceLayerPanel* TriggerLayer::getPanel()
{
	return new TriggerLayerPanel(this);
}

SequenceLayerTimeline* TriggerLayer::getTimelineUI()
{
	return new TriggerLayerTimeline(this);
}
