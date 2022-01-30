/*
  ==============================================================================

	LayerGroup.cpp
	Created: 30 Jan 2021 3:08:41pm
	Author:  Emerick Hervé

  ==============================================================================
*/

LayerGroupTimeline::LayerGroupTimeline(LayerGroup* layer) :
	SequenceLayerTimeline(layer),
	layerGroup(layer)
{
	timelineManagerUI.reset(new SequenceLayerTimelineManagerUI(layer->layerManager.get()));
	addAndMakeVisible(timelineManagerUI.get());

	needle.toFront(false);
	//updateContent();
}

LayerGroupTimeline::~LayerGroupTimeline()
{

}

void LayerGroupTimeline::resized()
{
	timelineManagerUI->setBounds(getLocalBounds().withTrimmedTop(layerGroup->headerHeight - 3));
}

void LayerGroupTimeline::updateContent()
{
}

void LayerGroupTimeline::addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables)
{
}
