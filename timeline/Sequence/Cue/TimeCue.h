/*
  ==============================================================================

    TimeCue.h
    Created: 6 Apr 2017 11:13:23am
    Author:  Ben

  ==============================================================================
*/

class Sequence;
class TimeCueManager;

class TimeCue :
	public BaseItem
{
public:
	TimeCue(const float &_time = 0, TimeCueManager * manager = nullptr);
	virtual ~TimeCue();

	FloatParameter * time;

	enum CueAction { NOTHING, PAUSE, LOOP_JUMP };
	EnumParameter * cueAction;
	TargetParameter* loopCue;


	virtual bool isCurrentlyActive(); // can be overriden by children 

	virtual void onContainerParameterChangedInternal(Parameter* p) override;

	Sequence * getSequence();
};
