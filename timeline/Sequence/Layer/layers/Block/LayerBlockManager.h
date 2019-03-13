/*
  ==============================================================================

    LayerBlockManager.h
    Created: 14 Feb 2019 11:14:28am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class LayerBlockManager :
	public BaseManager<LayerBlock>
{
public:
	LayerBlockManager(StringRef name = "Blocks");
	~LayerBlockManager();
	
	bool blocksCanOverlap;
	 Array<Point<float>> computeEmptySpaces(LayerBlock * exludeBlock);

	LayerBlock * addBlockAt(float time);
	LayerBlock * getBlockAtTime(float time, bool returnClosestPreviousIfNotFound = false, bool includeDisabled = true);
	LayerBlock * getNextBlockAtTime(float time, bool includeDisabled = true);
	Array<LayerBlock *> getBlocksAtTime(float time, bool includeDisabled = true);
	Array<LayerBlock *> getBlocksInRange(float start, float end, bool includeDisabled = true);

	void placeBlockAt(LayerBlock * block, float desiredTime);

	static int compareTime(LayerBlock * t1, LayerBlock * t2);
};
 