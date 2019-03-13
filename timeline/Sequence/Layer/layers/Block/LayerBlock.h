/*
  ==============================================================================

    LayerBlock.h
    Created: 14 Feb 2019 11:14:35am
    Author:  bkupe

  ==============================================================================
*/

#pragma once


class LayerBlock :
	public BaseItem
{
public:
	LayerBlock(StringRef name = "Block", float time = 0);
	~LayerBlock();

	FloatParameter * time;
	FloatParameter * coreLength;
	FloatParameter * loopLength;
	BoolParameter * isActive;

	float getTotalLength();
	float getCoreEndTime();
	float getEndTime();
	bool isInRange(float time);

	virtual void setCoreLength(float newLength, bool stretch, bool stickToCoreEnd = false);
	virtual void setLoopLength(float newLength);
	virtual void setStartTime(float newStart, bool keepCoreEnd = false, bool stickToCoreEnd = false);
	
	virtual double getRelativeTime(double t, bool timeIsAbsolute = false, bool noLoop = false);
};