/*
  ==============================================================================

    AudioLayerClipManager.cpp
    Created: 8 Feb 2017 12:19:53pm
    Author:  Ben

  ==============================================================================
*/


AudioLayerClipManager::AudioLayerClipManager(AudioLayer * layer) :
	LayerBlockManager(layer, "Clips"),
	audioLayer(layer)
{
	itemDataType = "AudioClip";
	blocksCanOverlap = false;

	isSelectable = false;
}

AudioLayerClipManager::~AudioLayerClipManager()
{
}

LayerBlock* AudioLayerClipManager::createItem()
{
	return audioLayer->createAudioClip();
}
