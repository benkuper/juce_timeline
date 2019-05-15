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
		ttmui.reset(new TimeTriggerManagerUI(this, layer->ttm.get()));
		addAndMakeVisible(ttmui.get());
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