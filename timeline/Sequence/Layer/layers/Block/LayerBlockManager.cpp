#include "LayerBlockManager.h"
/*
  ==============================================================================

	LayerBlockManager.cpp
	Created: 14 Feb 2019 11:14:28am
	Author:  bkupe

  ==============================================================================
*/

LayerBlockManager::LayerBlockManager(StringRef name) :
	BaseManager(name),
	blocksCanOverlap(true)
{

	hideInEditor = true;
	comparator.compareFunc = &LayerBlockManager::compareTime;
}

LayerBlockManager::~LayerBlockManager()
{
}


Array<Point<float>> LayerBlockManager::computeEmptySpaces(LayerBlock * excludeBlock)
{
	Array<Point<float>> result;

	if (items.size() == 0) return result;

	float lastEndTime = 0;
	for (auto &i : items)
	{
		if (i == excludeBlock) continue;

		result.add({ lastEndTime, i->time->floatValue() });
		lastEndTime = i->getEndTime();
	}

	result.add({ lastEndTime, (float)INT32_MAX });

	return result;
}

LayerBlock * LayerBlockManager::addBlockAt(float time)
{
	LayerBlock * t = createItem();
	t->time->setValue(time);
	
	BaseManager::addItem(t);

	placeBlockAt(t, time);

	int nextIndex = items.indexOf(t) + 1;
	if (nextIndex < items.size())
	{

		if (t->getEndTime() > items[nextIndex]->time->floatValue()) t->setCoreLength(items[nextIndex]->time->floatValue() - t->time->floatValue(), false);
	}
	

	return t;
}

LayerBlock * LayerBlockManager::getBlockAtTime(float time, bool returnClosestPreviousIfNotFound, bool includeDisabled)
{
	LayerBlock * closestPrevious = nullptr;
	for (auto &c : items)
	{
		if (!includeDisabled && !c->enabled->boolValue()) continue;

		if (c->isInRange(time)) return c;
		if (c->time->floatValue() < time) closestPrevious = c;
	}

	return returnClosestPreviousIfNotFound ? closestPrevious : nullptr;
}

LayerBlock * LayerBlockManager::getNextBlockAtTime(float time, bool includeDisabled)
{
	LayerBlock * closestNext = nullptr;
	for (int i=items.size()-1;i>=0;i--)
	{
		if (!includeDisabled && !items[i]->enabled->boolValue()) continue;


		if (items[i]->isInRange(time)) return items[i];
		if (items[i]->time->floatValue() > time) closestNext = items[i];
	}

	return closestNext;
}

Array<LayerBlock*> LayerBlockManager::getBlocksAtTime(float time, bool includeDisabled)
{
	Array<LayerBlock*> result;
	for (auto &c : items)
	{
		if (!includeDisabled && !c->enabled->boolValue()) continue;
		if (c->isInRange(time)) result.add(c);
	}
	return result;
}


Array<LayerBlock*> LayerBlockManager::getBlocksInRange(float start, float end, bool includeDisabled)
{
	Array<LayerBlock*> result;
	for (auto &c : items)
	{
		if (!includeDisabled && !c->enabled->boolValue()) continue;
		if (c->getEndTime() >= start || c->time->floatValue() <= end) result.add(c);
	}
	return result;
}

void LayerBlockManager::placeBlockAt(LayerBlock * block, float desiredTime)
{
	if (blocksCanOverlap)
	{
		block->time->setValue(desiredTime);
		return;
	}

	Array<Point<float>> emptySpaces = computeEmptySpaces(block);

	float targetTime = emptySpaces[emptySpaces.size() - 1].x;

	//Get closest empty space
	float minDist = INT32_MAX;
	Point<float> space;
	for (auto &p : emptySpaces)
	{
		if (desiredTime >= p.x && desiredTime <= p.y)
		{
			space = p;
			break;
		}

		float dist = jmin<float>(fabs(desiredTime - p.x), fabsf(desiredTime - p.y));
		if (dist < minDist)
		{
			space = p;
			minDist = dist;
		}
	}

	
	bool isEnoughSpace = space.y - space.x >= block->getTotalLength();
	if (isEnoughSpace)
	{
		if (desiredTime <= space.x) targetTime = space.x;
		else if (desiredTime + block->getTotalLength() >= space.y) targetTime = space.y-block->getTotalLength();
		else targetTime = desiredTime;

		block->time->setValue(targetTime);
	}
}

int LayerBlockManager::compareTime(LayerBlock * t1, LayerBlock * t2)
{
	if (t1->time->floatValue() < t2->time->floatValue()) return -1;
	else if (t1->time->floatValue() > t2->time->floatValue()) return 1;
	return 0;
}
