/*
  ==============================================================================

	SequenceBlockLayer.cpp
	Created: 20 Nov 2016 3:08:41pm
	Author:  Ben Kuper

  ==============================================================================
*/
#include "JuceHeader.h"

SequenceBlockLayer::SequenceBlockLayer(Sequence* _sequence, var params) :
	SequenceLayer(_sequence, getTypeString()),
	blockManager(this, dynamic_cast<SequenceManager*>(Engine::mainEngine->getControllableContainerForAddress(params.getProperty("manager", "")))),
	currentBlock(nullptr)
{

	addChildControllableContainer(&blockManager);
	blockManager.addBaseManagerListener(this);
}

SequenceBlockLayer::~SequenceBlockLayer()
{
	clearItem();
}

void SequenceBlockLayer::getSnapTimes(Array<float>* arrayToFill)
{
	blockManager.getSnapTimes(arrayToFill);
}

void SequenceBlockLayer::clearItem()
{
	blockManager.clear();
	SequenceLayer::clearItem();
}

void SequenceBlockLayer::itemAdded(LayerBlock* item)
{
	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) return;
	updateCurrentBlock();
}

void SequenceBlockLayer::itemsAdded(Array<LayerBlock*> items)
{
	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) return;
	updateCurrentBlock();
}

void SequenceBlockLayer::itemRemoved(LayerBlock* item)
{
	if (isClearing) return;
	updateCurrentBlock();
}
void SequenceBlockLayer::itemsRemoved(Array<LayerBlock*> items)
{
	if (isClearing) return;
	updateCurrentBlock();
}

void SequenceBlockLayer::updateCurrentBlock()
{
	SequenceBlock* b = (SequenceBlock*)blockManager.getBlockAtTime(sequence->currentTime->floatValue(), false, false);

	ScopedLock lock(blockLock);

	if (b == currentBlock) return;

	if (!currentBlockRef.wasObjectDeleted() && currentBlock != nullptr)
	{
		currentBlock->isActive->setValue(false);
		if (Sequence* s = currentBlock->getTargetSequence()) s->pauseTrigger->trigger();
	}

	if (b != nullptr)
	{
		if (Sequence* s = b->getTargetSequence())
		{
			if (s == sequence)
			{
				LOGWARNING("Cannot control the sequence from within itself, this is how you create black holes !");
				return;
			}
		}
	}


	currentBlock = b;
	currentBlockRef = b;

	if (!currentBlockRef.wasObjectDeleted() && currentBlock != nullptr)
	{

		currentBlock->isActive->setValue(true);

		updateCurrentSequenceTime();
		if (sequence->isPlaying->boolValue())
		{
			if (Sequence* s = currentBlock->getTargetSequence()) s->playTrigger->trigger();
		}
	}
}
void SequenceBlockLayer::updateCurrentSequenceTime()
{
	ScopedLock lock(blockLock);

	if (currentBlock == nullptr) return;

	if (Sequence* s = currentBlock->getTargetSequence())
	{
		float t = currentBlock->getRelativeTime(sequence->currentTime->floatValue(), true);

		if (sequence->isPlaying->boolValue())
		{
			if (sequence->isSeeking || t < s->currentTime->floatValue())
			{
				s->setCurrentTime(t, true, true);
				s->playTrigger->trigger();
			}
		}
		else
		{
			s->setCurrentTime(t, true, true);
		}
	}
}

void SequenceBlockLayer::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	SequenceLayer::onControllableFeedbackUpdateInternal(cc, c);

	if (!currentBlockRef.wasObjectDeleted() && currentBlock != nullptr)
	{
		if (c == currentBlock->enabled || c == currentBlock->coreLength || c == currentBlock->loopLength)
		{
			updateCurrentBlock();
			updateCurrentSequenceTime();
		}
		else if (c == currentBlock->time) updateCurrentSequenceTime();

	}
}

void SequenceBlockLayer::selectAll(bool addToSelection)
{
	blockManager.askForSelectAllItems(addToSelection);
	setSelected(false);
}

var SequenceBlockLayer::getJSONData()
{
	var data = SequenceLayer::getJSONData();
	data.getDynamicObject()->setProperty(blockManager.shortName, blockManager.getJSONData());

	return data;
}

void SequenceBlockLayer::loadJSONDataInternal(var data)
{
	SequenceLayer::loadJSONDataInternal(data);
	blockManager.loadJSONData(data.getProperty(blockManager.shortName, var()));
}

SequenceLayerPanel* SequenceBlockLayer::getPanel()
{
	return new SequenceBlockLayerPanel(this);
}

SequenceLayerTimeline* SequenceBlockLayer::getTimelineUI()
{
	return new SequenceBlockLayerTimeline(this);
}

void SequenceBlockLayer::sequenceCurrentTimeChanged(Sequence*, float, bool)
{
	updateCurrentBlock();
	updateCurrentSequenceTime();
}

void SequenceBlockLayer::sequencePlayStateChanged(Sequence*)
{
	if (!sequence->isPlaying->boolValue())
	{
		ScopedLock lock(blockLock);

		if (currentBlock != nullptr)
		{
			if (Sequence* s = currentBlock->getTargetSequence()) s->pauseTrigger->trigger();
		}
	}
	else
	{
		ScopedLock lock(blockLock);

		if (currentBlock != nullptr && enabled->boolValue())
		{
			if (Sequence* s = currentBlock->getTargetSequence())
			{
				updateCurrentSequenceTime();
				s->playTrigger->trigger();
			}
		}
	}
}