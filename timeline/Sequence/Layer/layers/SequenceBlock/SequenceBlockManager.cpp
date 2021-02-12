/*
  ==============================================================================

    SequenceBlockManager.cpp
    Created: 8 Feb 2017 12:19:53pm
    Author:  Ben

  ==============================================================================
*/


SequenceBlockManager::SequenceBlockManager(SequenceBlockLayer * layer, SequenceManager * refManager) :
	LayerBlockManager(layer, "Blocks"),
	sequenceBlockLayer(layer),
	refManager(refManager)
{
	jassert(refManager != nullptr);

	itemDataType = "SequenceBlock";
	blocksCanOverlap = false;

	isSelectable = false;
}

SequenceBlockManager::~SequenceBlockManager()
{
}

LayerBlock* SequenceBlockManager::createItem()
{
	return new SequenceBlock(refManager);
}
