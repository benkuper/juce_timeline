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
	bgColor = AUDIO_COLOR.withSaturation(.2f).darker(1);

	enveloppeUI.reset(audioLayer->enveloppe->createSlider());
	addAndMakeVisible(enveloppeUI.get());

}

AudioLayerPanel::~AudioLayerPanel()
{
}


void AudioLayerPanel::resizedInternalContent(Rectangle<int>& r)
{
	SequenceLayerPanel::resizedInternalContent(r);  
	Rectangle<int> gr = r.removeFromTop(16).reduced(2);
	enveloppeUI->setBounds(gr);
}