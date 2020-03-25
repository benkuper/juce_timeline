/*
  ==============================================================================

    SequenceLayer.h
    Created: 28 Oct 2016 8:15:05pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceLayerPanel;
class SequenceLayerTimeline;

class SequenceLayer :
	public BaseItem,
	public Sequence::SequenceListener
{
public:
	SequenceLayer(Sequence * _sequence = nullptr, const String &name = "New Layer");
	virtual ~SequenceLayer();

	Sequence * sequence;

	//UI
	IntParameter * uiHeight;
	ColorParameter * color;


	virtual void addDefaultContent() {}

	//Factory
	virtual String getTypeString() const override { jassert(false); return ""; } //should be overriden

	//UI
	virtual SequenceLayerPanel * getPanel();
	virtual SequenceLayerTimeline * getTimelineUI();

	//InspectableEditor * getEditor(bool isRoot) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceLayer)
};