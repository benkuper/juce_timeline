/*
  ==============================================================================

    TriggerLayerTimeline.cpp
    Created: 20 Nov 2016 3:08:29pm
    Author:  Ben Kuper

  ==============================================================================
*/

TriggerLayerTimeline::TriggerLayerTimeline(TriggerLayer * layer) :
	SequenceLayerTimeline(layer)
{
	if (layer->ttm != nullptr)
	{
		ttmui = new TimeTriggerManagerUI(this, layer->ttm);
		addAndMakeVisible(ttmui);
	}

	updateContent();
}

TriggerLayerTimeline::~TriggerLayerTimeline()
{
}

void TriggerLayerTimeline::resized()
{
	if(ttmui != nullptr) ttmui->setBounds(getLocalBounds());
}

void TriggerLayerTimeline::updateContent()
{
	if(ttmui != nullptr) ttmui->updateContent();
}