/*
  ==============================================================================

    AudioLayerClipManager.cpp
    Created: 8 Feb 2017 12:19:53pm
    Author:  Ben

  ==============================================================================
*/


AudioLayerClipManager::AudioLayerClipManager(AudioLayer * layer) :
	LayerBlockManager(layer)
{
	itemDataType = "AudioClip";
	blocksCanOverlap = false;
}

AudioLayerClipManager::~AudioLayerClipManager()
{
}

LayerBlock* AudioLayerClipManager::createItem()
{
	return new AudioLayerClip();
}
