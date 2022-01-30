/*
  ==============================================================================

    SequenceLayerManager.cpp
    Created: 28 Oct 2016 8:15:28pm
    Author:  bkupe

  ==============================================================================
*/

SequenceLayerManager::SequenceLayerManager(Sequence * _sequence, LayerGroup* _layerGroup) :
	BaseManager<SequenceLayer>("Layers"),
	sequence(_sequence),
	layerGroup(_layerGroup)
{
	itemDataType = "SequenceLayer";
	if (_layerGroup == nullptr)
	{
		hideInEditor = true;
		managerFactory = new Factory<SequenceLayer>();
	}
	else
	{
		managerFactory = _sequence->layerManager->managerFactory;
	}
}

SequenceLayerManager::~SequenceLayerManager()
{
}

void SequenceLayerManager::fileDropped(String file)
{
	if (file.endsWith("mp3") || file.endsWith("wav") || file.endsWith("aiff")) createAudioLayerForFile(file);
}

void SequenceLayerManager::getAllItems(Array<SequenceLayer*> * list)
{
	for (auto& l : items)
	{
		if (l->isGroup)
		{
			l->getAllItems(list);
		}
		else
		{
			list->add(l);
		}
	}
}

SequenceLayer * SequenceLayerManager::createItem()
{
	return new SequenceLayer(sequence);
}

void SequenceLayerManager::createAudioLayerForFile(File f)
{
	for (auto &d: managerFactory->defs)
	{
		if (((LayerDefinition*)d)->isAudio)
		{
			AudioLayer * layer = (AudioLayer *)managerFactory->create(d);
			AudioLayerClip* clip = new AudioLayerClip();
			clip->filePath->setValue(f.getFullPathName());
			layer->clipManager.addItem(clip, false, false);
			addItem(layer, true, true);
			return;
		}
	}
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
