/*
  ==============================================================================

	SequenceLayerManager.cpp
	Created: 28 Oct 2016 8:15:28pm
	Author:  bkupe

  ==============================================================================
*/

#include "JuceHeader.h"

SequenceLayerManager::SequenceLayerManager(Sequence* _sequence) :
	BaseManager<SequenceLayer>("Layers"),
	sequence(_sequence)
{
	itemDataType = "SequenceLayer";
	hideInEditor = true;
	managerFactory = &factory;

	hideInRemoteControl = true;
	defaultHideInRemoteControl = true;
}

SequenceLayerManager::~SequenceLayerManager()
{
}

void SequenceLayerManager::fileDropped(String file)
{
	if (file.endsWith("mp3") || file.endsWith("wav") || file.endsWith("aiff")) createAudioLayerForFile(file);

#if JUCE_WINDOWS
	if (file.endsWith("mp4") || file.endsWith("mov") || file.endsWith("avi") || file.endsWith("mkv") || file.endsWith("wmv") || file.endsWith("webm") || file.endsWith("m4v")) createVideoLayerForFile(file);
#endif
}

SequenceLayer* SequenceLayerManager::createItem()
{
	return new SequenceLayer(sequence);
}

void SequenceLayerManager::createAudioLayerForFile(File f)
{
	for (auto& d : factory.defs)
	{
		if (((LayerDefinition*)d)->isAudio)
		{
			AudioLayer* layer = (AudioLayer*)factory.create(d);
			AudioLayerClip* clip = new AudioLayerClip();
			clip->filePath->setValue(f.getFullPathName());
			layer->clipManager.addItem(clip, false, false);
			addItem(layer, true, true);
			return;
		}
	}
}

void SequenceLayerManager::createVideoLayerForFile(File f)
{
#if JUCE_WINDOWS
	for (auto& d : factory.defs)
	{
		if (((LayerDefinition*)d)->isVideo)
		{
			VideoLayer* layer = (VideoLayer*)factory.create(d);
			VideoLayerClip* clip = layer->createVideoClip();
			clip->filePath->setValue(f.getFullPathName());
			layer->clipManager.addItem(clip, false, false);
			addItem(layer, true, true);
			return;
		}
	}
#endif
}

#if TIMELINE_UNIQUE_LAYER_FACTORY
SequenceLayer* SequenceLayerManager::addItemFromData(var data, bool addToUndo)
{
	String layerType = data.getProperty("type", "none");
	if (layerType.isEmpty()) return nullptr;
	SequenceLayer* i = SequenceLayerFactory::getInstance()->createSequenceLayer(sequence, layerType);
	if (i != nullptr) return addItem(i, data, fromUndoableAction);
	return nullptr;
}
Array<SequenceLayer*> SequenceLayerManager::addItemsFromData(var data, bool addToUndo)
{
	Array<SequenceLayer*> itemsToAdd;
	for (int i = 0; i < data.size(); i++)
	{
		String layerType = data.getProperty("type", "none");
		if (layerType.isEmpty()) continue;
		SequenceLayer* i = SequenceLayerFactory::getInstance()->createSequenceLayer(sequence, layerType);
		if (i != nullptr) itemsToAdd.add(i);
	}

	return addItems(itemsToAdd, data, addToUndo);
}
#endif
