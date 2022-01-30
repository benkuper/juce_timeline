/*
  ==============================================================================

	LayerGroup.cpp
	Created: 30 Jan 2021 3:08:41pm
	Author:  Emerick Hervé

  ==============================================================================
*/

#pragma once

class LayerGroupTimeline :
	public SequenceLayerTimeline
{
public:
	LayerGroupTimeline(LayerGroup * layer);
	~LayerGroupTimeline();

	LayerGroup * layerGroup;
	std::unique_ptr <SequenceLayerTimelineManagerUI> timelineManagerUI;


	void resized() override;
	void updateContent() override;

	virtual void addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables) override;

};
