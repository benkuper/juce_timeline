/*
  ==============================================================================

    VideoLayerPanel.h
    Created: 26 Sep 2026

  ==============================================================================
*/

#pragma once

class VideoLayerPanel :
	public SequenceLayerPanel
{
public:
	VideoLayerPanel(VideoLayer* layer);
	~VideoLayerPanel();

	VideoLayer* videoLayer;

	std::unique_ptr<FloatSliderUI> volumeUI;

	void resizedInternalHeader(Rectangle<int>& r) override;
	void resizedInternalContent(Rectangle<int>& r) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoLayerPanel)
};