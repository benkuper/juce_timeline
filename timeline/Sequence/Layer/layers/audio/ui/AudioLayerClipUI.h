/*
  ==============================================================================

    AudioLayerClipUI.h
    Created: 8 Feb 2017 12:20:09pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class AudioLayerClipUI :
	public LayerBlockUI,
	public AudioLayerClip::AsyncListener
{
public:
	AudioLayerClipUI(AudioLayerClip * clip);
	~AudioLayerClipUI();

	AudioThumbnailCache thumbnailCache;
	AudioThumbnail thumbnail;
	AudioLayerClip * clip;

	void paint(Graphics &g) override;

	virtual void setupThumbnail();

	virtual void controllableFeedbackUpdateInternal(Controllable *) override;

	virtual void newMessage(const AudioLayerClip::ClipEvent &e) override;

};