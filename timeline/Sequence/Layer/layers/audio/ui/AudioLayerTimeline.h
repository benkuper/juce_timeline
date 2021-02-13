/*
  ==============================================================================

    AudioLayerTimeline.h
    Created: 20 Nov 2016 3:09:01pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class AudioLayerTimeline :
	public SequenceLayerTimeline
{
public:
	AudioLayerTimeline(AudioLayer * layer);
	~AudioLayerTimeline();

	AudioLayer * audioLayer;
	std::unique_ptr<AudioLayerClipManagerUI> cmMUI;


	void resized() override;

	void updateContent() override;
	virtual void updateMiniModeUI() override;

	virtual void addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables) override;

	
};