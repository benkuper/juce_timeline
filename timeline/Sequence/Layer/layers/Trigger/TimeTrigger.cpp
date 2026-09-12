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

	setHasCustomColor(true);
	itemColor->setDefaultValue(BG_COLOR.brighter(.2f));

	showWarningInUI = true;

	time = addFloatParameter("Time", "Time at which the action is triggered", 0, 0);

	time->defaultUI = FloatParameter::TIME;
	flagY = addFloatParameter("Flag Y", "Position of the trigger's flag", 0,0,1);
	isTriggered = addBoolParameter("Is Triggered", "Is this Time Trigger already triggered during this playing ?", false);
	length = addFloatParameter("Length", "Time before the deactivation of the trigger, put 0 to disable automatic deactivation", 0, 0);
	length->defaultUI = FloatParameter::TIME;

	isTriggered->setEnabled(false);
	isTriggered->isSavable = false;
	canTrigger = addBoolParameter("Can trigger", "If false the trigger is blocked and cannot trigger", true);
	canTrigger->hideInEditor = true;
	canTrigger->isSavable = false;
	triggerAtAnyTime = false;
	collisionState = false;
}

TimeTrigger::~TimeTrigger()
{

}


void TimeTrigger::setMovePositionReferenceInternal()
{
	movePositionReference = Point<float>(time->floatValue(), flagY->floatValue());
}

void TimeTrigger::setPosition(Point<float> targetPosition)
{
	time->setValue(targetPosition.x);
	flagY->setValue(targetPosition.y);
}

Point<float> TimeTrigger::getPosition()
{
	return Point<float>(time->floatValue(), flagY->floatValue());
}

void TimeTrigger::addUndoableMoveAction(Array<UndoableAction*>& actions)
{
	actions.add(time->setUndoableValue(movePositionReference.x, time->floatValue(), true));
	actions.add(flagY->setUndoableValue(movePositionReference.y, flagY->floatValue(), true));

}

void TimeTrigger::trigger()
{
	if (!enabled->boolValue() || !canTrigger->boolValue() || isTriggered->boolValue()) return;
	isTriggered->setValue(true);
	triggerInternal();
}

void TimeTrigger::unTrigger()
{
	if (!isTriggered->boolValue()) return;
	isTriggered->setValue(false);
	if (enabled->boolValue()) unTriggerInternal();
}

void TimeTrigger::setTriggerState(bool state, bool rewind)
{
	collisionState = state;
	if (state) trigger();
	else exitedInternal(rewind);
}

void TimeTrigger::updateTriggerState()
{
	if (triggerAtAnyTime || !collisionState) trigger();
	collisionState = true;
}
