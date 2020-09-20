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

	isUILocked->hideInEditor = false;

	pauseOnCue = addBoolParameter("Pause On Cue", "If checked, the sequence will pause when the time is on this cue", false);
}

TimeCue::~TimeCue()
{
}

Sequence * TimeCue::getSequence()
{
	ControllableContainer * cc = parentContainer;

	while (cc != nullptr)
	{
		Sequence * s = dynamic_cast<Sequence *>(cc);
		if (s != nullptr) return s;
		cc = cc->parentContainer;
	}

	return nullptr;
}
