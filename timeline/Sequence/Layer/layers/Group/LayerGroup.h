/*
  ==============================================================================

	LayerGroup.cpp
	Created: 30 Jan 2021 3:08:41pm
	Author:  Emerick Hervé

  ==============================================================================
*/

#pragma once

class LayerGroupProcessor;

class LayerGroup :
	public SequenceLayer,
	public Inspectable::InspectableListener
{
public:
	LayerGroup(Sequence * sequence, var params);
	~LayerGroup();

	std::unique_ptr<SequenceLayerManager> layerManager;
	int headerHeight;

	virtual void getAllItems(Array<SequenceLayer*> * l) override;
	virtual void clearItem() override;

	void selectAll(bool addToSelection = false) override;

	virtual var getJSONData() override;
	virtual void loadJSONDataInternal(var data) override;

	virtual SequenceLayerPanel * getPanel() override;
	virtual SequenceLayerTimeline * getTimelineUI() override;

	static LayerGroup * create(Sequence * sequence, var params) { return new LayerGroup(sequence, params); }
	virtual String getTypeString() const override { return "Group"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerGroup)
};
