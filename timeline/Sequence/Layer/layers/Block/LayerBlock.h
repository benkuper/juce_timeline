/*
  ==============================================================================

    LayerBlock.h
    Created: 14 Feb 2019 11:14:35am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "BlockTransitions.h"


class LayerBlock :
	public BaseItem
{
public:
	LayerBlock(StringRef name = "Block");
	~LayerBlock();

	FloatParameter * time;
	FloatParameter * coreLength;
	FloatParameter * loopLength;
	BoolParameter * isActive;

	// Optional per-block overrides. Disabled edges use the layer's automatic fades.
	FloatParameter* blockFadeIn = nullptr;
	FloatParameter* blockFadeOut = nullptr;
	void addFadeParameters(double defaultDuration = 0);
	virtual BlockTransitions::Fades getEffectiveFades() const;
	virtual double getFadeCurveValue(double weight) const { return weight; }

	float getTotalLength();
	float getCoreEndTime();
	float getEndTime();

	bool isInRange(float time);

	void setMovePositionReferenceInternal() override;
	void setPosition(Point<float> targetPosition) override;
	Point<float> getPosition() override;

	void addUndoableMoveAction(Array<UndoableAction *> &actions) override;

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
