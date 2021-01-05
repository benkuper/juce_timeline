/*
  ==============================================================================

    AudioLayerClipManager.h
    Created: 8 Feb 2017 12:19:53pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class AudioLayer;

class AudioLayerClipManager :
	public LayerBlockManager
{
public:
	AudioLayerClipManager(AudioLayer * layer);
	~AudioLayerClipManager();

	AudioLayer* audioLayer;
	LayerBlock* createItem() override;
	
};
