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
		ttmui->getSnapTimesFunc = std::bind(&TriggerLayer::getSequenceSnapTimesForManager, layer, std::placeholders::_1);

		addAndMakeVisible(ttmui.get());
	}

	updateContent();
	updateMiniModeUI();
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

void TriggerLayerTimeline::updateMiniModeUI()
{
	if (ttmui != nullptr) ttmui->setMiniMode(item->miniMode->boolValue());
}

void TriggerLayerTimeline::addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables)
{
	if (ttmui == nullptr) return;
	ttmui->addSelectableComponentsAndInspectables(selectables, inspectables);
}
