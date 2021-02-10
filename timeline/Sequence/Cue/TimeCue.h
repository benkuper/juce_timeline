/*
  ==============================================================================

    TimeCue.h
    Created: 6 Apr 2017 11:13:23am
    Author:  Ben

  ==============================================================================
*/

class Sequence;

class TimeCue :
	public BaseItem
{
public:
	TimeCue(const float &_time = 0);
	~TimeCue();

	FloatParameter * time;

	enum CueAction { NOTHING, PAUSE, LOOP_JUMP };
	EnumParameter * cueAction;
	TargetParameter* loopCue;

	void setParentContainer(ControllableContainer* container) override;

	void onContainerParameterChangedInternal(Parameter* p) override;

	Sequence * getSequence();
};
