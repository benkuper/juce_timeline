/*
  ==============================================================================

	TimeTriggerManager.h
	Created: 10 Dec 2016 12:22:48pm
	Author:  Ben

  ==============================================================================
*/

#pragma once
#include <map>
#include <set>
#include <vector>

class TriggerLayer;


class TimeTriggerManager :
	public BaseManager<TimeTrigger>,
	public Sequence::SequenceListener
{
public:
	TimeTriggerManager(TriggerLayer* layer, Sequence* sequence);
	virtual ~TimeTriggerManager();

	TriggerLayer* layer;
	Sequence* sequence;

	virtual void addTriggerAt(float time, float flagYPos);

	void addItemInternal(TimeTrigger* t, var data) override;
	void addItemsInternal(Array<TimeTrigger*> items, var data) override;
	void removeItemInternal(TimeTrigger*) override;
	void reorderItems() override;
	void setItemIndex(TimeTrigger*, int newIndex, bool addToUndo = true) override;

	Array<TimeTrigger*> addItemsFromClipboard(bool showWarning = true) override;
	bool canAddItemOfType(const String& typeToCheck) override;

	TimeTrigger* getPrevTrigger(float time, bool includeCurrentTime = false);
	TimeTrigger* getNextTrigger(float time, bool includeCurrentTime = false);
	Array<TimeTrigger*> getTriggersInTimespan(float startTime, float endTime, bool includeAlreadyTriggered = false);

	Array<UndoableAction*> getMoveKeysBy(float start, float offset);
	Array<UndoableAction*> getRemoveTimespan(float start, float end);

	void onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c) override;
	void executeTriggersTimespan(float startTime, float endTime, bool forward, bool onlyUntrigger = false);

	void sequenceCurrentTimeChanged(Sequence* _sequence, float prevTime, bool evaluateSkippedData) override;
	void sequenceTimeChanged(Sequence*, const Sequence::TimeChange&) override;
	bool processTimeChange(const Sequence::TimeChange&, bool includeFrom = false);
	void triggerAllConsequences(bool state);
	void sequencePlayStateChanged(Sequence*) override;
	void sequenceTotalTimeChanged(Sequence*) override;
	void sequencePlayDirectionChanged(Sequence*) override;
	void sequenceLooped(Sequence*) override;

	static int compareTime(TimeTrigger* t1, TimeTrigger* t2);

private:
	struct Boundary { WeakReference<ControllableContainer> trigger; bool enter; std::size_t order; };
	using BoundaryIndex = std::multimap<double, Boundary>;
	struct IndexedTrigger { BoundaryIndex::iterator start, end; bool hasEnd; std::size_t order; };
	BoundaryIndex boundaries;
	std::map<TimeTrigger*, IndexedTrigger> indexedTriggers;
	std::set<TimeTrigger*> activeDurations;
	std::set<TimeTrigger*> durationTriggers;
	std::size_t nextOrder = 0;
	std::uint64_t dispatchRevision = 0;
	double pointCursorTime = -1;
	std::set<TimeTrigger*> pointsAtCursor;
	void indexTrigger(TimeTrigger*);
	void reconcileTrigger(TimeTrigger*, double time);
	void refreshBoundaryOrder();
	void reconcileDurations(double time);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeTriggerManager)
};
