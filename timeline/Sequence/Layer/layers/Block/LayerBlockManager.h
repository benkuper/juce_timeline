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
	public Manager<LayerBlock>,
	public LayerBlock::BlockListener
{
public:
	LayerBlockManager(SequenceLayer* layer, StringRef name = "Blocks");
	~LayerBlockManager();

	SequenceLayer* layer;

	bool blocksCanOverlap;
	Array<Point<float>> computeEmptySpaces(LayerBlock* exludeBlock);

	LayerBlock* addBlockAt(float time);
	void addBlockAt(LayerBlock * b, float time);
	LayerBlock* getBlockAtTime(float time, bool returnClosestPreviousIfNotFound = false, bool includeDisabled = true);
	LayerBlock* getNextBlockAtTime(float time, bool includeDisabled = true);
	Array<LayerBlock*> getBlocksAtTime(float time, bool includeDisabled = true);
	Array<LayerBlock*> getBlocksInRange(float start, float end, bool includeDisabled = true);

	Array<LayerBlock*> addItemsFromClipboard(bool showWarning = true) override;

	virtual void getSnapTimes(Array<float>* arrayToFill, bool includeStart = true, bool includeEnd = true, bool includeCoreEnd = false);

	void addItemInternal(LayerBlock* item, var) override;
	void addItemsInternal(Array<LayerBlock*> items, var) override;
	void removeItemInternal(LayerBlock* item) override;
	void removeItemsInternal(Array<LayerBlock*> items) override;

	void askForPlaceBlockTime(LayerBlock* block, float desiredTime) override;
	void placeBlockAt(LayerBlock* block, float desiredTime);

	static int compareTime(LayerBlock* t1, LayerBlock* t2);
};

