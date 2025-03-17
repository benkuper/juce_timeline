/*
  ==============================================================================

    TimeTrigger.h
    Created: 20 Nov 2016 3:18:20pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class TimeTrigger :
	public BaseItem
{
public:
	TimeTrigger(StringRef name = "Trigger");
	virtual ~TimeTrigger();

	FloatParameter * time;
	BoolParameter * isTriggered;
	FloatParameter * length;

	BoolParameter * canTrigger;
	bool triggerAtAnyTime;
	bool collisionState;

	//ui
	FloatParameter * flagY;

	void setMovePositionReferenceInternal() override;
	void setPosition(Point<float> targetTime) override;
	Point<float> getPosition() override;

	void addUndoableMoveAction(Array<UndoableAction *> &actions) override;

	virtual void trigger();
	virtual void triggerInternal() {}

	virtual void unTrigger();
	virtual void unTriggerInternal() {}

	virtual void exitedInternal(bool rewind) {}

	void setTriggerState(bool state, bool rewind = false);
	void updateTriggerState();

	DECLARE_TYPE("TimeTrigger");
};