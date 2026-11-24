/*
  ==============================================================================

    VideoLayerClipUI.h
    Created: 26 Sep 2026

  ==============================================================================
*/

#pragma once

class VideoLayerClipUI :
	public LayerBlockUI,
	public VideoLayerClip::ClipListener
{
public:
	VideoLayerClipUI(VideoLayerClip* clip);
	~VideoLayerClipUI();

	VideoLayerClip* clip;

	void paint(Graphics& g) override;
	void resizedBlockInternal() override;
	void controllableFeedbackUpdateInternal(Controllable* c) override;
	void clipSourceLoaded(VideoLayerClip*) override;
	void clipThumbnailChanged(VideoLayerClip*) override;

private:

};
