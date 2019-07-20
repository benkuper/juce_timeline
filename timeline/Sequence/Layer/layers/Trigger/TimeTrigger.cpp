#include "TimeTrigger.h"
/*
  ==============================================================================

    TimeTrigger.cpp
    Created: 20 Nov 2016 3:18:20pm
    Author:  Ben Kuper

  ==============================================================================
*/

TimeTrigger::TimeTrigger(StringRef name) :
	BaseItem(name)
{
	itemDataType = "TimeTrigger"; 
	showWarningInUI = true;

	time = addFloatParameter("Time", "Time at which the action is triggered", 0, 0, 3600);
	
	time->defaultUI = FloatParameter::TIME;
	flagY = addFloatParameter("Flag Y", "Position of the trigger's flag", 0,0,1);
	isTriggered = addBoolParameter("Is Triggered", "Is this Time Trigger already triggered during this playing ?", false);
	

	isLocked = addBoolParameter("Locked", "When locked, you can't change time or flag values by dragging it", false);

	isTriggered->hideInEditor = true;
	isTriggered->isSavable = false;
}

TimeTrigger::~TimeTrigger()
{

}

void TimeTrigger::onContainerParameterChangedInternal(Parameter * p)
{
	if (p == isLocked)
	{
		time->setControllableFeedbackOnly(isLocked->boolValue());
		//flagY->setControllableFeedbackOnly(isLocked->boolValue());
	}
}

void TimeTrigger::setMoveTimeReferenceInternal()
{
	moveTimeReference = time->floatValue();
}

void TimeTrigger::setTime(float targetTime)
{
	time->setValue(targetTime);
}

UndoableAction* TimeTrigger::getUndoableMoveAction()
{
	return time->setUndoableValue(moveTimeReference, time->floatValue(), true);
}

void TimeTrigger::trigger()
{
	if (!enabled->boolValue()) return;
	isTriggered->setValue(true);
	triggerInternal();
}