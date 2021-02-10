/*
  ==============================================================================

    TimeCue.cpp
    Created: 6 Apr 2017 11:13:23am
    Author:  Ben

  ==============================================================================
*/

TimeCue::TimeCue(const float & _time) :
	BaseItem("Cue")
{
	//nameParam->hideInEditor = false;
	helpID = "TimeCue";

	time = addFloatParameter("Time", "Cue Time", 0, 0);
	time->setValue(_time, true, true);
	time->defaultUI = FloatParameter::TIME;

	cueAction = addEnumParameter("Action on Cue", "This can be used to add an extra action when the timeline hits the cue. Pause will pause at the cue, Loop Jump will jump to another cue, allowing for easy loop in-out behaviour.");
	cueAction->addOption("Nothing", NOTHING)->addOption("Pause", PAUSE)->addOption("Loop Jump", LOOP_JUMP);

	loopCue = addTargetParameter("Loop Jump Cue", "If Cue Action is set to Loop Jump, this will decide which Cue to jump to", nullptr, false);
	loopCue->targetType = TargetParameter::CONTAINER;
	loopCue->maxDefaultSearchLevel = 0;
	loopCue->showParentNameInEditor = false;

	isUILocked->hideInEditor = false;

}

TimeCue::~TimeCue()
{
}

void TimeCue::setParentContainer(ControllableContainer* container)
{
	BaseItem::setParentContainer(container);

	if (parentContainer != nullptr)
	{
		loopCue->setRootContainer(parentContainer);
	}
}

void TimeCue::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == cueAction)
	{
		loopCue->setEnabled(cueAction->getValueDataAsEnum<CueAction>() == LOOP_JUMP);
	}
	else if (p == loopCue)
	{
		if (loopCue->targetContainer == this)
		{
			NLOGWARNING(niceName, "Auto referencing this cue for Loop Jump, are you looking to create a spacetime breach ??");
		}
	}
}

Sequence * TimeCue::getSequence()
{
	if (parentContainer == nullptr) return nullptr;
	return dynamic_cast<Sequence*>(parentContainer->parentContainer.get());
}
