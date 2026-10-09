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
	enum SeekEvaluation { inheritSeek = -1, neverSeek = 0, playingSeek = 1, stoppedSeek = 2, alwaysSeek = 3 };
	EnumParameter* forwardSeek;
	EnumParameter* backwardSeek;

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
	virtual void setTimelineActive(bool active, bool evaluate, bool rewind = false);
	virtual void evaluateCurrentVisit() { updateTriggerState(); }
	virtual void dispatchConsequences(bool state);
	virtual bool replayForwardSeek() const { return false; }
	bool shouldEvaluateSeek(bool forward, bool playing, bool inherited) const;

	DECLARE_TYPE("TimeTrigger");
};
