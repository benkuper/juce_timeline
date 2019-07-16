/*
  ==============================================================================

    AudioLayerClipManager.h
    Created: 8 Feb 2017 12:19:53pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class AudioLayerClipManager :
	public BaseManager<AudioLayerClip>
{
public:
	AudioLayerClipManager();
	~AudioLayerClipManager();

	AudioLayerClip * addClipAt(float time);

	AudioLayerClip * getClipAtTime(float time, bool includeDisabled = false);
};
