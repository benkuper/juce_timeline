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
	auto addSeek = [this](const String& name)
	{
		auto* p = addEnumParameter(name, "Override the sequence's Evaluate on Seek setting for this direction");
		p->addOption("Inherit", inheritSeek)->addOption("Always", alwaysSeek)->addOption("Playing only", playingSeek)
			->addOption("Stopped only", stoppedSeek)->addOption("Never", neverSeek);
		return p;
	};
	forwardSeek = addSeek("Forward seek");
	backwardSeek = addSeek("Backward seek");
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
	setTimelineActive(state, true, rewind);
}

void TimeTrigger::setTimelineActive(bool active, bool evaluate, bool rewind)
{
	collisionState = active;
	if (!evaluate) return;
	if (active) trigger();
	else exitedInternal(rewind);
}

void TimeTrigger::dispatchConsequences(bool state)
{
	if (!enabled->boolValue() || isClearing) return;
	if (auto* layer = ControllableUtil::findParentAs<SequenceLayer>(this);
		layer && (!layer->enabled->boolValue() || !layer->sequence->enabled->boolValue())) return;
	WeakReference<ControllableContainer> safeThis(this);
	isTriggered->setValue(state);
	if (safeThis == nullptr) return;
	if (state) triggerInternal();
	else unTriggerInternal();
}

bool TimeTrigger::shouldEvaluateSeek(bool forward, bool playing, bool inherited) const
{
	auto mode = (forward ? forwardSeek : backwardSeek)->getValueDataAsEnum<SeekEvaluation>();
	return mode == inheritSeek ? inherited : mode == alwaysSeek || (mode == playingSeek && playing) || (mode == stoppedSeek && !playing);
}

void TimeTrigger::updateTriggerState()
{
	if (triggerAtAnyTime || !collisionState) trigger();
	collisionState = true;
}
