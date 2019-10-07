/*
  ==============================================================================

    LayerBlock.h
    Created: 14 Feb 2019 11:14:35am
    Author:  bkupe

  ==============================================================================
*/

#pragma once


class LayerBlock :
	public BaseItem,
	public TimedLayerItem
{
public:
	LayerBlock(StringRef name = "Block", float time = 0);
	~LayerBlock();

	FloatParameter * time;
	FloatParameter * coreLength;
	FloatParameter * loopLength;
	BoolParameter * isActive;
	BoolParameter* isLocked;

	float getTotalLength();
	float getCoreEndTime();
	float getEndTime();
	bool isInRange(float time);

	void setMoveTimeReferenceInternal() override;
	void setTime(float targetTime) override;
	float getTime() override;

	UndoableAction *getUndoableMoveAction() override;

	virtual void setCoreLength(float newLength, bool stretch, bool stickToCoreEnd = false);
	virtual void setLoopLength(float newLength);
	virtual void setStartTime(float newStart, bool keepCoreEnd = false, bool stickToCoreEnd = false);
	
	virtual double getRelativeTime(double t, bool timeIsAbsolute = false, bool noLoop = false);

	class BlockListener
	{
	public:
		virtual ~BlockListener() {}
		virtual void askForPlaceBlockTime(LayerBlock *, float desiredTime) {}
	};

	ListenerList<BlockListener> blockListeners;
	void addBlockListener(BlockListener* newListener) { blockListeners.add(newListener); }
	void removeBlockListener(BlockListener* listener) { blockListeners.remove(listener); }

};