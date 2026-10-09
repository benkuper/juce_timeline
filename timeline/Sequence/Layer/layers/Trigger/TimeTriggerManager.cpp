/*
  ==============================================================================

	TimeTriggerManager.cpp
	Created: 10 Dec 2016 12:22:48pm
	Author:  Ben

  ==============================================================================
*/

#include "JuceHeader.h"
#include <map>

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
	indexTrigger(t);
	if (items.getLast() != t && !isManipulatingMultipleItems) refreshBoundaryOrder();
}

void TimeTriggerManager::addItemsInternal(Array<TimeTrigger*> items, var data)
{
	for (auto& t : items) { t->time->setRange(0, sequence->totalTime->floatValue()); indexTrigger(t); }
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
	for (auto& t : triggers) actions.add(t->time->setUndoableValue(t->time->floatValue(), t->time->floatValue() + offset, true));
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


void TimeTriggerManager::removeItemInternal(TimeTrigger* trigger)
{
    ++dispatchRevision;
    auto it = indexedTriggers.find(trigger);
    if (it != indexedTriggers.end())
    {
        boundaries.erase(it->second.start);
        if (it->second.hasEnd) boundaries.erase(it->second.end);
        indexedTriggers.erase(it);
    }
    activeDurations.erase(trigger);
    durationTriggers.erase(trigger);
    pointsAtCursor.erase(trigger);
}

void TimeTriggerManager::reconcileTrigger(TimeTrigger* trigger, double current)
{
    if (trigger->length->floatValue() <= 0) { activeDurations.erase(trigger); return; }
    const bool active = current >= trigger->time->doubleValue()
        && current < trigger->time->doubleValue() + trigger->length->doubleValue();
    if (active) activeDurations.insert(trigger); else activeDurations.erase(trigger);
    trigger->setTimelineActive(active, false);
}

void TimeTriggerManager::indexTrigger(TimeTrigger* trigger)
{
    const auto old = indexedTriggers.find(trigger);
    const auto order = old == indexedTriggers.end() ? nextOrder++ : old->second.order;
    removeItemInternal(trigger);
    auto start = boundaries.emplace(trigger->time->doubleValue(), Boundary { trigger, true, order });
    const bool hasEnd = trigger->length->floatValue() > 0;
    if (hasEnd) durationTriggers.insert(trigger);
    auto end = hasEnd ? boundaries.emplace(trigger->time->doubleValue() + trigger->length->doubleValue(),
        Boundary { trigger, false, order }) : boundaries.end();
    indexedTriggers.emplace(trigger, IndexedTrigger { start, end, hasEnd, order });
    reconcileTrigger(trigger, sequence->currentTime->doubleValue());
}

void TimeTriggerManager::reconcileDurations(double time)
{
    for (auto* trigger : durationTriggers) reconcileTrigger(trigger, time);
}

void TimeTriggerManager::reorderItems()
{
    // Equal-time cues keep their saved order, including during bulk load/paste.
    items.sort(comparator, true);
    controllableContainers.clear();
    controllableContainers.addArray(items);
    baseManagerListeners.call(&ManagerListener::itemsReordered);
    managerNotifier.addMessage(new ManagerEvent(ManagerEvent::ITEMS_REORDERED));
    refreshBoundaryOrder();
}

void TimeTriggerManager::refreshBoundaryOrder()
{
    ++dispatchRevision;
    std::size_t order = 0;
    for (auto* trigger : items)
    {
        auto it = indexedTriggers.find(trigger);
        if (it == indexedTriggers.end()) continue;
        it->second.order = order;
        it->second.start->second.order = order;
        if (it->second.hasEnd) it->second.end->second.order = order;
        ++order;
    }
    nextOrder = order;
}

void TimeTriggerManager::setItemIndex(TimeTrigger* trigger, int newIndex, bool addToUndo)
{
    BaseManager::setItemIndex(trigger, newIndex, addToUndo);
    refreshBoundaryOrder();
}

void TimeTriggerManager::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
    auto* trigger = dynamic_cast<TimeTrigger*>(cc);
    if (trigger == nullptr || !items.contains(trigger)) return;
    if (c == trigger->time || c == trigger->length)
    {
        indexTrigger(trigger);
        if (c == trigger->time) reorderItems();
    }
}

void TimeTriggerManager::executeTriggersTimespan(float startTime, float endTime, bool forward, bool onlyUntrigger)
{
    processTimeChange({ forward ? startTime : endTime, forward ? endTime : startTime,
        onlyUntrigger ? Sequence::TimeChangeKind::Seek : Sequence::TimeChangeKind::Playback,
        sequence->isPlaying->boolValue(), true, sequence->transportRevision.load() });
}

bool TimeTriggerManager::processTimeChange(const Sequence::TimeChange& change, bool includeFrom)
{
    if (isClearing || sequence->isClearing || change.revision != sequence->transportRevision.load()) return false;
    const bool enabled = layer->enabled->boolValue() && sequence->enabled->boolValue()
        && !sequence->isCurrentlyLoadingData && !isCurrentlyLoadingData
        && !(Engine::mainEngine && Engine::mainEngine->isLoadingFile);
    const bool forward = change.currentTime > change.previousTime
        || (change.currentTime == change.previousTime && (!includeFrom || sequence->playSpeed->floatValue() >= 0));
    const bool seek = change.kind == Sequence::TimeChangeKind::Seek;
    const double low = jmin(change.previousTime, change.currentTime);
    const double high = jmax(change.previousTime, change.currentTime);
    struct Action { double time; Boundary boundary; bool active; bool point; bool rewind; bool evaluate; };
    std::vector<Action> actions;
    std::set<TimeTrigger*> syncDurations;
    auto eligible = [&](TimeTrigger* t)
    {
        return enabled && t->enabled->boolValue()
            && (!seek || t->shouldEvaluateSeek(forward, change.playing,
                change.evaluateSkippedData || ModifierKeys::getCurrentModifiers().isCtrlDown()));
    };
    // A reverse wrap can start inside a block extending beyond the sequence end,
    // without crossing that block's end boundary. Keep these entries in the same
    // ordered event stream as point cues so transport-changing cues cancel them.
    if (includeFrom && !forward)
        for (auto* trigger : durationTriggers)
            if (!trigger->collisionState && change.previousTime >= trigger->time->doubleValue()
                && change.previousTime < trigger->time->doubleValue() + trigger->length->doubleValue())
            {
                const auto indexed = indexedTriggers.find(trigger);
                if (indexed != indexedTriggers.end())
                    actions.push_back({ change.previousTime, { trigger, true, indexed->second.order },
                        true, false, false, eligible(trigger) });
            }
    for (auto it = boundaries.lower_bound(low); it != boundaries.end() && it->first <= high; ++it)
    {
        auto* trigger = dynamic_cast<TimeTrigger*>(it->second.trigger.get());
        if (!trigger) continue;
        const bool point = trigger->length->floatValue() <= 0;
        const double time = it->first;
        if (point)
        {
            const bool crossed = forward ? (time > change.previousTime && time <= change.currentTime)
                : seek ? (time > change.currentTime && time <= change.previousTime)
                : (time >= change.currentTime && time < change.previousTime);
            if (crossed || (includeFrom && time == change.previousTime
                && (pointCursorTime != time || !pointsAtCursor.count(trigger))))
                actions.push_back({ time, it->second, true, true, seek && !forward, eligible(trigger) });
        }
        else if (seek && !(forward && trigger->replayForwardSeek()))
            syncDurations.insert(trigger);
        else
        {
            const bool crossed = forward ? (time > change.previousTime && time <= change.currentTime)
                : (time <= change.previousTime && time > change.currentTime);
            if (crossed || (forward && time == change.previousTime && it->second.enter && !trigger->collisionState)
                || (includeFrom && forward && time == change.previousTime))
                actions.push_back({ time, it->second, forward ? it->second.enter : !it->second.enter,
                    false, false, eligible(trigger) });
        }
    }
    if (seek)
    {
        // A consequence can seek before an earlier span has finished dispatching.
        // Query all duration blocks at the destination, including those not entered yet.
        // Point cues remain indexed and are never scanned here.
        for (auto* trigger : durationTriggers)
        {
            const double start = trigger->time->doubleValue();
            const double end = start + trigger->length->doubleValue();
            const bool crossedBoundary = (start > change.previousTime && start <= change.currentTime)
                || (end > change.previousTime && end <= change.currentTime);
            if (!(forward && trigger->replayForwardSeek() && crossedBoundary)) syncDurations.insert(trigger);
        }
        for (auto* trigger : syncDurations)
        {
            const bool active = change.currentTime >= trigger->time->doubleValue()
                && change.currentTime < trigger->time->doubleValue() + trigger->length->doubleValue();
            if (active == trigger->collisionState) continue;
            const auto indexed = indexedTriggers.find(trigger);
            if (indexed == indexedTriggers.end()) continue;
            const double boundaryTime = trigger->time->doubleValue()
                + ((forward != active) ? trigger->length->doubleValue() : 0.0);
            actions.push_back({ boundaryTime, { trigger, active, indexed->second.order },
                active, false, !forward && !active, eligible(trigger) });
        }
    }
    std::stable_sort(actions.begin(), actions.end(), [forward](const Action& a, const Action& b)
    {
        if (a.time != b.time) return forward ? a.time < b.time : a.time > b.time;
        return a.boundary.order < b.boundary.order;
    });
    // Selection precedes condition evaluation: an invalid latest cue runs its FALSE block.
    if (seek && forward && (int)layer->forwardSeekPoints->getValueData() == 1)
    {
        auto last = actions.end();
        for (auto it = actions.begin(); it != actions.end(); ++it) if (it->point && it->evaluate) last = it;
        for (auto it = actions.begin(); it != actions.end(); ++it)
            if (it->point && it != last) it->evaluate = false;
    }
    const auto generation = ++dispatchRevision;
    if (pointCursorTime != change.currentTime) pointsAtCursor.clear();
    pointCursorTime = change.currentTime;
    WeakReference<ControllableContainer> safeThis(this), safeSequence(sequence);
    for (const auto& action : actions)
    {
        auto* trigger = dynamic_cast<TimeTrigger*>(action.boundary.trigger.get());
        if (!trigger || !indexedTriggers.count(trigger)) continue;
        if (!action.point)
        {
            if (action.active) activeDurations.insert(trigger); else activeDurations.erase(trigger);
        }
        else if (action.rewind) pointsAtCursor.erase(trigger);
        else if (action.evaluate && action.time == pointCursorTime) pointsAtCursor.insert(trigger);
        trigger->setTimelineActive(action.active && !action.rewind, action.evaluate, action.rewind);
        if (safeThis == nullptr || safeSequence == nullptr) return false;
        if (isClearing || sequence->isClearing || dispatchRevision != generation
            || sequence->transportRevision.load() != change.revision)
        {
            if (!isClearing && !sequence->isClearing) reconcileDurations(sequence->currentTime->doubleValue());
            return false;
        }
    }
    // Visits initialized silently by edits/loading can evaluate on the next playback update.
    if (!seek && enabled)
    {
        std::vector<WeakReference<ControllableContainer>> active;
        for (auto* trigger : activeDurations) active.emplace_back(trigger);
        for (const auto& weak : active)
        {
            if (auto* trigger = dynamic_cast<TimeTrigger*>(weak.get())) trigger->evaluateCurrentVisit();
            if (safeThis == nullptr || safeSequence == nullptr) return false;
            if (dispatchRevision != generation || sequence->transportRevision.load() != change.revision)
            { reconcileDurations(sequence->currentTime->doubleValue()); return false; }
        }
    }
    return true;
}

void TimeTriggerManager::sequenceTimeChanged(Sequence*, const Sequence::TimeChange& change)
{
    if (change.kind != Sequence::TimeChangeKind::Loop) { processTimeChange(change); return; }
    const bool forward = sequence->playSpeed->floatValue() >= 0;
    const double boundary = forward ? sequence->totalTime->doubleValue() : 0.0;
    WeakReference<ControllableContainer> safeThis(this), safeSequence(sequence);
    if (!processTimeChange({ change.previousTime, boundary, Sequence::TimeChangeKind::Playback,
        change.playing, true, change.revision })) return;
    if (safeThis == nullptr || safeSequence == nullptr || sequence->transportRevision.load() != change.revision) return;
    std::vector<WeakReference<ControllableContainer>> outgoing;
    for (auto* t : activeDurations) outgoing.emplace_back(t);
    activeDurations.clear();
    const auto generation = dispatchRevision;
    for (auto weak : outgoing)
    {
        if (auto* trigger = dynamic_cast<TimeTrigger*>(weak.get()))
            trigger->setTimelineActive(false, layer->enabled->boolValue() && sequence->enabled->boolValue());
        if (safeThis == nullptr || safeSequence == nullptr || sequence->transportRevision.load() != change.revision) return;
        if (dispatchRevision != generation)
        { if (!isClearing && !sequence->isClearing) reconcileDurations(sequence->currentTime->doubleValue()); return; }
    }
    processTimeChange({ forward ? 0.0 : sequence->totalTime->doubleValue(), change.currentTime,
        Sequence::TimeChangeKind::Playback, change.playing, true, change.revision }, true);
}

void TimeTriggerManager::sequenceCurrentTimeChanged(Sequence*, float previous, bool evaluate)
{
    processTimeChange({ previous, sequence->currentTime->doubleValue(), sequence->isSeeking
        || !sequence->isPlaying->boolValue() ? Sequence::TimeChangeKind::Seek : Sequence::TimeChangeKind::Playback,
        sequence->isPlaying->boolValue(), evaluate, sequence->transportRevision.load() });
}

void TimeTriggerManager::sequencePlayStateChanged(Sequence*)
{
    if (!sequence->isPlaying->boolValue()) return;
    const double time = sequence->currentTime->doubleValue();
    processTimeChange({ time, time, Sequence::TimeChangeKind::Playback, true, true, sequence->transportRevision.load() }, true);
}

void TimeTriggerManager::sequenceTotalTimeChanged(Sequence*)
{
    for (auto* trigger : items) trigger->time->setRange(0, sequence->totalTime->floatValue());
}

void TimeTriggerManager::sequencePlayDirectionChanged(Sequence*) {}
void TimeTriggerManager::sequenceLooped(Sequence*) {} // The typed loop update owns visit completion.

void TimeTriggerManager::triggerAllConsequences(bool state)
{
    if (!layer->enabled->boolValue() || !sequence->enabled->boolValue()) return;
    std::vector<WeakReference<ControllableContainer>> snapshot;
    for (auto* trigger : items) snapshot.emplace_back(trigger);
    const auto generation = dispatchRevision;
    const auto revision = sequence->transportRevision.load();
    WeakReference<ControllableContainer> safeThis(this), safeSequence(sequence);
    for (const auto& weak : snapshot)
    {
        if (auto* trigger = dynamic_cast<TimeTrigger*>(weak.get())) trigger->dispatchConsequences(state);
        if (safeThis == nullptr || safeSequence == nullptr) return;
        if (dispatchRevision != generation || sequence->transportRevision.load() != revision) return;
    }
}

int TimeTriggerManager::compareTime(TimeTrigger* t1, TimeTrigger* t2)
{
    return t1->time->floatValue() < t2->time->floatValue() ? -1
        : t1->time->floatValue() > t2->time->floatValue() ? 1 : 0;
}
