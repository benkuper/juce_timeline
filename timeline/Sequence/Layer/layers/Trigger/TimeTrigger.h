/*
  ==============================================================================

    TimeTrigger.h
    Created: 20 Nov 2016 3:18:20pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class TimeTrigger :
	public BaseItem,
	public TimedLayerItem
{
public:
	TimeTrigger(StringRef name = "Trigger");
	virtual ~TimeTrigger();

	FloatParameter * time;
	BoolParameter * isTriggered;
	
	BoolParameter * isLocked;

	//ui
	FloatParameter * flagY;

	virtual void onContainerParameterChangedInternal(Parameter * p) override;

	void setMoveTimeReferenceInternal() override;
	void setTime(float targetTime) override;
	UndoableAction* getUndoableMoveAction() override;

	virtual void trigger();
	virtual void triggerInternal() {}
};