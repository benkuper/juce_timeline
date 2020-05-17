/*
  ==============================================================================

    TriggerLayer.cpp
    Created: 17 Nov 2016 7:59:54pm
    Author:  Ben Kuper

  ==============================================================================
*/

TriggerLayer::TriggerLayer(Sequence * _sequence, StringRef name, var params) :
	SequenceLayer(_sequence, String(name)),
	ttm(nullptr)
{
	helpID = "TriggerLayer";
	//color->setColor(ACTION_COLOR.darker());

	lockAll = addTrigger("Lock All", "Lock all existing triggers in this layer");
	unlockAll = addTrigger("Unlock All", "Unlock all existing triggers in this layer");
	triggerWhenSeeking = addBoolParameter("Trigger when seeking", "If checked, this when a sequence is playing and you jump on it forward, it will trigger all the triggers inbetween.", true);
}

TriggerLayer::~TriggerLayer()
{
}


void TriggerLayer::setManager(TimeTriggerManager * _ttm)
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
	Array<TimeTrigger *> p = ttm->addItemsFromClipboard(false);
	if (p.isEmpty()) return SequenceLayer::paste();
	return true;
}

void TriggerLayer::onContainerTriggerTriggered(Trigger * t)
{
	SequenceLayer::onContainerTriggerTriggered(t);
	
	if (ttm == nullptr) return;

	if (t == lockAll) for (auto &i : ttm->items) i->isUILocked->setValue(true);
	else if (t == unlockAll) for (auto &i : ttm->items) i->isUILocked->setValue(false);
}

var TriggerLayer::getJSONData()
{
	var data = SequenceLayer::getJSONData();
	if(ttm != nullptr) data.getDynamicObject()->setProperty(ttm->shortName, ttm->getJSONData());
	return data;
}

void TriggerLayer::loadJSONDataInternal(var data)
{
	SequenceLayer::loadJSONDataInternal(data);
	if(ttm != nullptr) ttm->loadJSONData(data.getProperty(ttm->shortName, var()));
}

SequenceLayerPanel * TriggerLayer::getPanel()
{
	return new TriggerLayerPanel(this);
}

SequenceLayerTimeline * TriggerLayer::getTimelineUI()
{
	return new TriggerLayerTimeline(this);
}
