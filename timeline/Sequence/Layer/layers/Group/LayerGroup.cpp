/*
  ==============================================================================

	LayerGroup.cpp
	Created: 30 Jan 2021 3:08:41pm
	Author:  Emerick Hervé

  ==============================================================================
*/


LayerGroup::LayerGroup(Sequence* _sequence, var params) :
	SequenceLayer(_sequence, "Group", true),
	headerHeight(24)
{
	helpID = "LayerGroup";

	layerManager.reset(new SequenceLayerManager(sequence, this));
	addChildControllableContainer(layerManager.get());
}

LayerGroup::~LayerGroup()
{
	clearItem();
}

void LayerGroup::getAllItems(Array<SequenceLayer*> * l)
{
	return layerManager->getAllItems(l);
}

void LayerGroup::clearItem()
{
	BaseItem::clearItem();
	//clipManager.clear();
	SequenceLayer::clearItem();
}

void LayerGroup::selectAll(bool addToSelection)
{
	setSelected(false);
}

var LayerGroup::getJSONData()
{
	var data = SequenceLayer::getJSONData();
	data.getDynamicObject()->setProperty("children", layerManager->getJSONData());

	return data;
}

void LayerGroup::loadJSONDataInternal(var data)
{
	layerManager->loadJSONData(data.getProperty("children", var()));
}

SequenceLayerPanel* LayerGroup::getPanel()
{
	return new LayerGroupPanel(this);
}

SequenceLayerTimeline* LayerGroup::getTimelineUI()
{
	return new LayerGroupTimeline(this);
}
