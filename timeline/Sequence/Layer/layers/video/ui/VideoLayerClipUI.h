/*
  ==============================================================================

    VideoLayerClipUI.h
    Created: 26 Sep 2026

  ==============================================================================
*/

#pragma once

class VideoLayerClipUI :
	public LayerBlockUI
{
public:
	VideoLayerClipUI(VideoLayerClip* clip);
	~VideoLayerClipUI();

	VideoLayerClip* clip;

	void paint(Graphics& g) override;
	void resizedBlockInternal() override;

private:

};