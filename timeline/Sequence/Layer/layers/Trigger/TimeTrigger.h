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

	//ui
	FloatParameter * flagY;

	void setMovePositionReferenceInternal() override;
	void setPosition(Point<float> targetTime) override;
	Point<float> getPosition() override;

	void addUndoableMoveAction(Array<UndoableAction *> &actions) override;

	virtual void trigger();
	virtual void triggerInternal() {}
};