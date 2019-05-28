/*
  ==============================================================================

    TimeCueManager.h
    Created: 6 Apr 2017 11:13:27am
    Author:  Ben

  ==============================================================================
*/

#pragma once

class TimeCueComparator
{
public:
	
};

class TimeCueManager :
	public BaseManager<TimeCue>
{
public:
	TimeCueManager();
	~TimeCueManager();

	virtual void addCueAt(float time);

	virtual Array<float> getAllCueTimes(float minTime = 0, float maxTime = 0);
	virtual float getNearestCueForTime(float time);
	
	virtual float getNextCueForTime(float time);
	virtual float getPrevCueForTime(float time, float goToPreviousThreshold);

	virtual void onControllableFeedbackUpdate(ControllableContainer * cc, Controllable * c) override;

	static int compareTime(TimeCue * t1, TimeCue * t2);
};