/*
  ==============================================================================

    AudioLayerClipManagerUI.h
    Created: 8 Feb 2017 12:20:16pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class AudioLayerTimeline;

class AudioLayerClipManagerUI :
	public LayerBlockManagerUI
{
public:
	AudioLayerClipManagerUI(AudioLayerTimeline * timeline, AudioLayerClipManager * manager);
	~AudioLayerClipManagerUI();

	virtual LayerBlockUI* createUIForItem(LayerBlock* item) override;

	void mouseDoubleClick(const MouseEvent &e) override;
	void addClipWithFileChooserAt(float position);
};