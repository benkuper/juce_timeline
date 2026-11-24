/*
  ==============================================================================

    VideoLayerPanel.cpp
    Created: 26 Sep 2026

  ==============================================================================
*/

VideoLayerPanel::VideoLayerPanel(VideoLayer* layer) :
	SequenceLayerPanel(layer),
	videoLayer(layer)
{
	volumeUI.reset(videoLayer->volume->createSlider());
	addAndMakeVisible(volumeUI.get());
	contentComponents.add(volumeUI.get());
}

VideoLayerPanel::~VideoLayerPanel()
{
}

void VideoLayerPanel::resizedInternalHeader(Rectangle<int>& r)
{
	SequenceLayerPanel::resizedInternalHeader(r);
}

void VideoLayerPanel::resizedInternalContent(Rectangle<int>& r)
{
	SequenceLayerPanel::resizedInternalContent(r);

	if (!item->miniMode->boolValue())
	{
		volumeUI->setBounds(r.removeFromTop(16).reduced(2));
	}
}