/*
  ==============================================================================

    LayerBlockManager.h
    Created: 14 Feb 2019 11:14:28am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class LayerBlockComparator
{
public:
	int compareElements(LayerBlock * t1, LayerBlock * t2)
	{
		if (t1->time->floatValue() < t2->time->floatValue()) return -1;
		else if (t1->time->floatValue() > t2->time->floatValue()) return 1;
		return 0;
	}
};

class LayerBlockManager :
	public BaseManager<LayerBlock>
{
public:
	LayerBlockManager(StringRef name = "Blocks");
	~LayerBlockManager();
	
	bool blocksCanOverlap;

	static LayerBlockComparator comparator;

	void reorderItems() override;

	 Array<Point<float>> computeEmptySpaces(LayerBlock * exludeBlock);

	void onControllableFeedbackUpdate(ControllableContainer * cc, Controllable * c) override;

	LayerBlock * addBlockAt(float time);
	LayerBlock * getBlockAtTime(float time, bool returnClosestPreviousIfNotFound = false, bool includeDisabled = true);
	LayerBlock * getNextBlockAtTime(float time, bool includeDisabled = true);
	Array<LayerBlock *> getBlocksAtTime(float time, bool includeDisabled = true);
	Array<LayerBlock *> getBlocksInRange(float start, float end, bool includeDisabled = true);

	void placeBlockAt(LayerBlock * block, float desiredTime);

};
 