/*
  ==============================================================================

    TimeTriggerManager.h
    Created: 10 Dec 2016 12:22:48pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class TriggerLayer;


class TimeTriggerManager :
	public BaseManager<TimeTrigger>,
	public Sequence::SequenceListener
{
public:
	TimeTriggerManager(TriggerLayer * layer, Sequence * sequence);
	virtual ~TimeTriggerManager();
	
	TriggerLayer * layer;
	Sequence * sequence;

	virtual void addTriggerAt(float time,float flagYPos);

	void addItemInternal(TimeTrigger * t, var data) override;
	
	Array<TimeTrigger *> addItemsFromClipboard(bool showWarning = true) override;
	bool canAddItemOfType(const String & typeToCheck) override;

	Array<TimeTrigger *> getTriggersInTimespan(float startTime, float endTime, bool includeAlreadyTriggered = false);

	Array<UndoableAction*> getMoveKeysBy(float start, float offset);
	Array<UndoableAction*> getRemoveTimespan(float start, float end);

	void onControllableFeedbackUpdate(ControllableContainer * cc, Controllable * c) override;

	void sequenceCurrentTimeChanged(Sequence * _sequence, float prevTime, bool evaluateSkippedData) override;
	void sequenceTotalTimeChanged(Sequence*) override;
	void sequencePlayDirectionChanged(Sequence *) override;
	void sequenceLooped(Sequence *) override;

	static int compareTime(TimeTrigger * t1, TimeTrigger * t2);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeTriggerManager)
};