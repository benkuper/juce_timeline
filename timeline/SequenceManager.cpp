/*
  ==============================================================================

    SequenceManager.cpp
    Created: 28 Oct 2016 8:13:04pm
    Author:  bkupe

  ==============================================================================
*/

#if TIMELINE_USE_SEQUENCEMANAGER_SINGLETON
juce_ImplementSingleton(SequenceManager)
#endif

SequenceManager::SequenceManager() :
	BaseManager("Sequences"),
	defaultLayerFactory(nullptr)
{
	itemDataType = "Sequence";
	helpID = "TimeMachine";

	playAllTrigger = addTrigger("Play All", "Play all sequences");
	stopAllTrigger = addTrigger("Stop All", "Stop all sequences");
	onlyOneSequencePlaying = addBoolParameter("Only one sequence playing", "If checked, as soon as one sequence is playing, all the other ones stop", false);
}

SequenceManager::~SequenceManager()
{
}


void SequenceManager::addItemInternal(Sequence* item, var data)
{
	if (defaultLayerFactory != nullptr) item->layerManager->managerFactory = defaultLayerFactory;
	item->addSequenceListener(this);
}

void SequenceManager::removeItemInternal(Sequence* item)
{
	item->removeSequenceListener(this);
}

void SequenceManager::onContainerTriggerTriggered(Trigger* t)
{
	BaseManager::onContainerTriggerTriggered(t);
	if (t == playAllTrigger)
	{
		for (auto& i : items) i->playTrigger->trigger();
	}else if (t == stopAllTrigger)
	{
		for (auto& i : items) i->stopTrigger->trigger();
	}
}

void SequenceManager::sequencePlayStateChanged(Sequence* s)
{
	if (Engine::mainEngine->isClearing) return;
	if (s->isPlaying->boolValue() && onlyOneSequencePlaying->boolValue())
	{
		for (auto& i : items) if (i != s) i->stopTrigger->trigger();
	}
}

Sequence * SequenceManager::showMenuAndGetSequence()
{
	PopupMenu menu;
	int numItems = items.size();
	for (int i = 0; i < numItems; ++i)
	{
		menu.addItem(1 + i, items[i]->niceName);
	}
	int result = menu.show();
	return getSequenceForItemID(result);
}

Sequence* SequenceManager::getSequenceForItemID(int itemID)
{
	if (itemID <= 0 || itemID > items.size()) return nullptr;
	return items[itemID - 1];
}

SequenceLayer* SequenceManager::showmMenuAndGetLayer()
{
	PopupMenu menu;
	for (int i = 0; i < items.size(); ++i)
	{
		PopupMenu sMenu;
		int numValues = items[i]->layerManager->items.size();
		for (int j = 0; j < numValues; j++)
		{
			SequenceLayer* c = items[i]->layerManager->items[j];
			sMenu.addItem(i * 1000 + j + 1, c->niceName);
		}
		menu.addSubMenu(items[i]->niceName, sMenu);
	}

	int result = menu.show();
	if (result == 0) return nullptr;
	return getLayerForItemID(result);
}

SequenceLayer* SequenceManager::getLayerForItemID(int itemID)
{
	if (itemID <= 0) return nullptr;
	int sequenceIndex = (int)floor((itemID - 1) / 1000);
	int layerIndex = (itemID - 1) % 1000;
	return items[sequenceIndex]->layerManager->items[layerIndex];
}

TimeCue* SequenceManager::showMenuAndGetCue()
{
	PopupMenu menu;
	for (int i = 0; i < items.size(); ++i)
	{
		PopupMenu sMenu;
		int numValues = items[i]->cueManager->items.size();
		for (int j = 0; j < numValues; j++)
		{
			TimeCue * c = items[i]->cueManager->items[j];
			sMenu.addItem(i * 1000 + j + 1, c->niceName);
		}
		menu.addSubMenu(items[i]->niceName, sMenu);
	}

	int result = menu.show();
	return getCueForItemID(result);
}

TimeCue * SequenceManager::getCueForItemID(int itemID)
{
	if (itemID <= 0) return nullptr;
	int moduleIndex = (int)floor((itemID - 1) / 1000);
	int valueIndex = (itemID - 1) % 1000;
	return items[moduleIndex]->cueManager->items[valueIndex];
}