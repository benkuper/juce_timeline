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
	}
	else if (t == stopAllTrigger)
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

void SequenceManager::showMenuAndGetSequence(ControllableContainer* startFromCC, std::function<void(Sequence*)> returnFunc)
{
	PopupMenu menu;
	int numItems = items.size();
	for (int i = 0; i < numItems; ++i) menu.addItem(1 + i, items[i]->niceName);

	menu.showMenuAsync(PopupMenu::Options(), [this, returnFunc](int result)
		{
			if (Sequence* s = this->getSequenceForItemID(result)) returnFunc(s);
		}
	);
}

Sequence* SequenceManager::getSequenceForItemID(int itemID)
{
	if (itemID <= 0 || itemID > items.size()) return nullptr;
	return items[itemID - 1];
}

void SequenceManager::showMenuAndGetLayer(ControllableContainer* startFromCC, std::function<void(SequenceLayer*)> returnFunc)
{

	auto getMenuForSequence = [this](Sequence* s)
	{
		PopupMenu sMenu;
		int numValues = s->layerManager->items.size();
		for (int j = 0; j < numValues; j++)
		{
			SequenceLayer* c = s->layerManager->items[j];
			sMenu.addItem(items.indexOf(s) * 1000 + j + 1, c->niceName);
		}

		return sMenu;
	};


	PopupMenu menu;
	if (Sequence* s = dynamic_cast<Sequence*>(startFromCC)) menu = getMenuForSequence(s);
	else for (auto& s : items) menu.addSubMenu(s->niceName, getMenuForSequence(s));

	menu.showMenuAsync(PopupMenu::Options(), [this, returnFunc](int result)
		{
			if (SequenceLayer* l = this->getLayerForItemID(result)) returnFunc(l);
		}
	);
}

SequenceLayer* SequenceManager::getLayerForItemID(int itemID)
{
	if (itemID <= 0) return nullptr;
	int sequenceIndex = (int)floor((itemID - 1) / 1000);
	int layerIndex = (itemID - 1) % 1000;
	return items[sequenceIndex]->layerManager->items[layerIndex];
}

void SequenceManager::showMenuAndGetCue(ControllableContainer* startFromCC, std::function<void(TimeCue*)> returnFunc)
{

	auto getMenuForSequence = [this](Sequence* s)
	{
		PopupMenu sMenu;
		int numValues = s->cueManager->items.size();
		for (int j = 0; j < numValues; j++)
		{
			TimeCue* c = s->cueManager->items[j];
			sMenu.addItem(items.indexOf(s) * 1000 + j + 1, c->niceName);
		}

		return sMenu;
	};


	PopupMenu menu;
	if (Sequence* s = dynamic_cast<Sequence*>(startFromCC)) menu = getMenuForSequence(s);
	else for (auto& s : items) menu.addSubMenu(s->niceName, getMenuForSequence(s));

	menu.showMenuAsync(PopupMenu::Options(), [this, returnFunc](int result)
		{
			if (TimeCue* t = this->getCueForItemID(result)) returnFunc(t);
		}
	);
}

TimeCue* SequenceManager::getCueForItemID(int itemID)
{
	if (itemID <= 0) return nullptr;
	int moduleIndex = (int)floor((itemID - 1) / 1000);
	int valueIndex = (itemID - 1) % 1000;
	return items[moduleIndex]->cueManager->items[valueIndex];
}

void SequenceManager::showMenuAndGetAudioLayer(ControllableContainer* startFromCC, std::function<void(AudioLayer*)> returnFunc)
{

	auto getMenuForSequence = [this](Sequence* s)
	{
		PopupMenu sMenu;
		int numValues = s->layerManager->items.size();
		for (int j = 0; j < numValues; j++)
		{
			if (AudioLayer* al = dynamic_cast<AudioLayer*>(s->layerManager->items[j]))
			{
				sMenu.addItem(items.indexOf(s) * 1000 + j + 1, al->niceName);

			}
		}

		return sMenu;
	};


	PopupMenu menu;
	if (Sequence* s = dynamic_cast<Sequence*>(startFromCC)) menu = getMenuForSequence(s);
	else for (auto& s : items) menu.addSubMenu(s->niceName, getMenuForSequence(s));

	menu.showMenuAsync(PopupMenu::Options(), [this, returnFunc](int result)
		{
			if (AudioLayer* l = this->getAudioLayerForItemID(result)) returnFunc(l);
		}
	);
}

AudioLayer* SequenceManager::getAudioLayerForItemID(int itemID)
{
	if (itemID <= 0) return nullptr;
	int sequenceIndex = (int)floor((itemID - 1) / 1000);
	int layerIndex = (itemID - 1) % 1000;
	return (AudioLayer*)items[sequenceIndex]->layerManager->items[layerIndex];
}

void SequenceManager::showMenuAndGetTrigger(ControllableContainer* startFromCC, std::function<void(TimeTrigger*)> returnFunc)
{
	Array<TimeTrigger*> ttItems;

	auto getMenuForLayer = [&ttItems](TriggerLayer* layer)
	{
		PopupMenu tm;
		for (auto& tt : layer->ttm->items)
		{
			ttItems.add(tt);
			tm.addItem(ttItems.size(), tt->niceName);
		}

		return tm;
	};

	auto getMenuForSequence = [getMenuForLayer, &ttItems](Sequence* s)
	{
		PopupMenu sMenu;
		int numValues = s->layerManager->items.size();
		for (int j = 0; j < numValues; j++)
		{
			if (TriggerLayer* tl = dynamic_cast<TriggerLayer*>(s->layerManager->items[j]))
			{
				sMenu.addSubMenu(tl->niceName, getMenuForLayer(tl));
			}
		}

		return sMenu;
	};




	PopupMenu menu;
	if (Sequence* s = dynamic_cast<Sequence*>(startFromCC)) menu = getMenuForSequence(s);
	else if (TriggerLayer* l = dynamic_cast<TriggerLayer*>(startFromCC)) menu = getMenuForLayer(l);
	else for (auto& s : items) menu.addSubMenu(s->niceName, getMenuForSequence(s));


	menu.showMenuAsync(PopupMenu::Options(), [ttItems, returnFunc](int result)
		{
			if (result == 0) return;
			returnFunc(ttItems[result - 1]);
		}
	);
}