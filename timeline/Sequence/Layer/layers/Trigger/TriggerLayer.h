/*
  ==============================================================================

    TriggerLayer.h
    Created: 17 Nov 2016 7:59:54pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class TriggerLayer :
	public SequenceLayer
{
public :
	TriggerLayer(Sequence * _sequence, StringRef name = "Trigger", var params = var());
	~TriggerLayer();

	std::unique_ptr<TimeTriggerManager> ttm;
	
	Trigger * lockAll;
	Trigger * unlockAll;
	BoolParameter * triggerWhenSeeking;

	void setManager(TimeTriggerManager * ttm);

	virtual bool paste() override;

	virtual void onContainerTriggerTriggered(Trigger *) override;

	var getJSONData() override;
	void loadJSONDataInternal(var data) override;

	static TriggerLayer * create(Sequence * sequence, var params) { return new TriggerLayer(sequence, "Trigger", params); }
	virtual String getTypeString() const override { return "Trigger"; }


	virtual SequenceLayerPanel * getPanel() override;
	virtual SequenceLayerTimeline * getTimelineUI() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerLayer)
};