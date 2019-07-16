/*
  ==============================================================================

    LayerBlockManager.h
    Created: 14 Feb 2019 11:14:28am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceLayer;

class LayerBlockManager :
	public BaseManager<LayerBlock>
{
public:
	LayerBlockManager(SequenceLayer* layer, StringRef name = "Blocks");
	~LayerBlockManager();

	SequenceLayer* layer;

	bool blocksCanOverlap;
	 Array<Point<float>> computeEmptySpaces(LayerBlock * exludeBlock);

	LayerBlock * addBlockAt(float time);
	LayerBlock * getBlockAtTime(float time, bool returnClosestPreviousIfNotFound = false, bool includeDisabled = true);
	LayerBlock * getNextBlockAtTime(float time, bool includeDisabled = true);
	Array<LayerBlock *> getBlocksAtTime(float time, bool includeDisabled = true);
	Array<LayerBlock *> getBlocksInRange(float start, float end, bool includeDisabled = true);

	Array<LayerBlock *> addItemsFromClipboard(bool showWarning = true) override;

	void placeBlockAt(LayerBlock * block, float desiredTime);

	static int compareTime(LayerBlock * t1, LayerBlock * t2);
};
 