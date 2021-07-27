/*
  ==============================================================================

    AudioLayerPanel.cpp
    Created: 20 Nov 2016 3:08:49pm
    Author:  Ben Kuper

  ==============================================================================
*/

AudioLayerPanel::AudioLayerPanel(AudioLayer * layer) :
	SequenceLayerPanel(layer),
	audioLayer(layer)
{
	volumeUI.reset(audioLayer->volume->createSlider());
	addAndMakeVisible(volumeUI.get());
	contentComponents.add(volumeUI.get());

	enveloppeUI.reset(audioLayer->enveloppe->createSlider());
	addAndMakeVisible(enveloppeUI.get());
}

AudioLayerPanel::~AudioLayerPanel()
{
}


void AudioLayerPanel::resizedInternalHeader(Rectangle<int>& r)
{
	SequenceLayerPanel::resizedInternalHeader(r);
	if (item->miniMode->boolValue()) enveloppeUI->setBounds(r);
}

void AudioLayerPanel::resizedInternalContent(Rectangle<int>& r)
{
	SequenceLayerPanel::resizedInternalContent(r);  
	if (!item->miniMode->boolValue())
	{
		volumeUI->setBounds(r.removeFromTop(16).reduced(2));
		enveloppeUI->setBounds(r.removeFromTop(16).reduced(2));
	}
}