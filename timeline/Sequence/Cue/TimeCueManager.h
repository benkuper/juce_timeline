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

	void addCueAt(float time);

	Array<float> getAllCueTimes(float minTime = 0, float maxTime = 0);
	float getNearestCueForTime(float time);
	
	float getNextCueForTime(float time);
	float getPrevCueForTime(float time, float goToPreviousThreshold);

    void onControllableFeedbackUpdate(ControllableContainer * cc, Controllable * c) override;

	static int compareTime(TimeCue * t1, TimeCue * t2);
};