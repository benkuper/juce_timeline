/*
  ==============================================================================

    VideoLayerClipManager.cpp
    Created: 26 Sep 2026

  ==============================================================================
*/

#include "JuceHeader.h"
#include "VideoLayerClipManager.h"
#include "VideoLayer.h"

VideoLayerClipManager::VideoLayerClipManager(VideoLayer* layer) :
	LayerBlockManager(layer),
	videoLayer(layer)
{
}

VideoLayerClipManager::~VideoLayerClipManager()
{
}

LayerBlock* VideoLayerClipManager::createItem()
{
	return videoLayer->createVideoClip();
}