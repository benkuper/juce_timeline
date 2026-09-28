/*
  ==============================================================================

    VideoLayerTimeline.h
    Created: 26 Sep 2026

  ==============================================================================
*/

#pragma once

class VideoLayerTimeline :
	public SequenceLayerTimeline
{
public:
	VideoLayerTimeline(VideoLayer* layer);
	~VideoLayerTimeline();

	VideoLayer* videoLayer;
	std::unique_ptr<VideoLayerClipManagerUI> cmMUI;

	void resized() override;

	void updateContent() override;
	virtual void updateMiniModeUI() override;

	virtual void addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables) override;
};