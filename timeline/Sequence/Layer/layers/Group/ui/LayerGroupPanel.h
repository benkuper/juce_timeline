/*
  ==============================================================================

	LayerGroup.cpp
	Created: 30 Jan 2021 3:08:41pm
	Author:  Emerick Hervé

  ==============================================================================
*/

#pragma once

class LayerGroupPanel :
	public SequenceLayerPanel
{
public:
	LayerGroupPanel(LayerGroup * layer);
	~LayerGroupPanel();

	LayerGroup * layerGroup;
	std::unique_ptr<SequenceLayerPanelManagerUI> panelManagerUI;

	void resizedInternalHeader(Rectangle<int>& r) override;
	void resizedInternalContent(Rectangle<int> &r) override;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerGroupPanel)
};
