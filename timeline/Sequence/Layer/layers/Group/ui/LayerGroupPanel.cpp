/*
  ==============================================================================

	LayerGroup.cpp
	Created: 30 Jan 2021 3:08:41pm
	Author:  Emerick Hervé

  ==============================================================================
*/

LayerGroupPanel::LayerGroupPanel(LayerGroup* layer) :
	SequenceLayerPanel(layer, true),
	layerGroup(layer)
{
	panelManagerUI.reset(new SequenceLayerPanelManagerUI(layer->layerManager.get()));
	addAndMakeVisible(panelManagerUI.get());
}

LayerGroupPanel::~LayerGroupPanel()
{
}


void LayerGroupPanel::resizedInternalHeader(Rectangle<int>& r)
{
	SequenceLayerPanel::resizedInternalHeader(r);

	layerGroup->headerHeight = r.getHeight();
}

void LayerGroupPanel::resizedInternalContent(Rectangle<int>& r)
{
	SequenceLayerPanel::resizedInternalContent(r);

	panelManagerUI->setBounds(r);
}
