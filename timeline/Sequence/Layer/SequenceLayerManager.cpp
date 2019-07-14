/*
  ==============================================================================

    SequenceLayerManager.cpp
    Created: 28 Oct 2016 8:15:28pm
    Author:  bkupe

  ==============================================================================
*/

SequenceLayerManager::SequenceLayerManager(Sequence * _sequence) :
	BaseManager<SequenceLayer>("Layers"),
	sequence(_sequence),
	masterAudioLayer(nullptr)
{
	itemDataType = "SequenceLayer";
	hideInEditor = true;
	managerFactory = &factory;
}

SequenceLayerManager::~SequenceLayerManager()
{
	setMasterAudioLayer(nullptr);
}

SequenceLayer * SequenceLayerManager::createItem()
{
	return new SequenceLayer(sequence);
}

#if TIMELINE_UNIQUE_LAYER_FACTORY
SequenceLayer * SequenceLayerManager::addItemFromData(var data, bool fromUndoableAction)
{
	String layerType = data.getProperty("type", "none");
	if (layerType.isEmpty()) return nullptr;
	SequenceLayer * i = SequenceLayerFactory::getInstance()->createSequenceLayer(sequence, layerType);
	if (i != nullptr) return addItem(i, data, fromUndoableAction);
	return nullptr;
}
#endif


void SequenceLayerManager::updateTargetAudioLayer(AudioLayer * excludeLayer)
{
	if (masterAudioLayer == nullptr)
	{
		for (auto &i : items)
		{
			if (i == excludeLayer) continue;
		}

		setMasterAudioLayer(nullptr);
	}
	else
	{
	}	
}

void SequenceLayerManager::setMasterAudioLayer(AudioLayer * layer)
{
	masterAudioLayer = layer;
}

