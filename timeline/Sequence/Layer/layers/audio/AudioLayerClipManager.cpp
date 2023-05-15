/*
  ==============================================================================

    AudioLayerClipManager.cpp
    Created: 8 Feb 2017 12:19:53pm
    Author:  Ben

  ==============================================================================
*/

#include "JuceHeader.h"

AudioLayerClipManager::AudioLayerClipManager(AudioLayer * layer) :
	LayerBlockManager(layer, "Clips"),
	audioLayer(layer)
{
	itemDataType = AudioLayerClip::getTypeStringStatic();
	blocksCanOverlap = false;

	//isSelectable = false;
}

AudioLayerClipManager::~AudioLayerClipManager()
{
}

LayerBlock* AudioLayerClipManager::createItem()
{
	return audioLayer->createAudioClip();
}
