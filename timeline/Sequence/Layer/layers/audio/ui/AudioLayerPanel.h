/*
  ==============================================================================

    AudioLayerPanel.h
    Created: 20 Nov 2016 3:08:49pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class AudioLayerPanel :
	public SequenceLayerPanel
{
public:
	AudioLayerPanel(AudioLayer * layer);
	~AudioLayerPanel();

	AudioLayer * audioLayer;

	std::unique_ptr<FloatSliderUI> volumeUI;
	std::unique_ptr<FloatSliderUI> enveloppeUI;

	void resizedInternalHeader(Rectangle<int>& r) override;
	void resizedInternalContent(Rectangle<int> &r) override;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLayerPanel)
};