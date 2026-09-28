/*
  ==============================================================================

    VideoLayerClipManager.h
    Created: 26 Sep 2026

  ==============================================================================
*/

#pragma once

class VideoLayer;

class VideoLayerClipManager :
	public LayerBlockManager
{
public:
	VideoLayerClipManager(VideoLayer* layer);
	~VideoLayerClipManager();

	VideoLayer* videoLayer;
	LayerBlock* createItem() override;

};