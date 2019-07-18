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

	//interaction
	float timeAtMouseDown;
	int posAtMouseDown;

	AudioThumbnailCache thumbnailCache;
	AudioThumbnail thumbnail;
	AudioLayerClip * clip;

	void paint(Graphics &g) override;
	void resizedBlockInternal() override;



	void controllableFeedbackUpdateInternal(Controllable *) override;

	void newMessage(const AudioLayerClip::ClipEvent &e) override;

};