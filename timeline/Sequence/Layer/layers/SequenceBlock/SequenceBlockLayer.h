/*
  ==============================================================================

    SequenceBlockLayer.h
    Created: 20 Nov 2016 3:08:41pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class SequenceBlockLayer :
	public SequenceLayer,
	public SequenceBlockManager::ManagerListener
{
public:
	SequenceBlockLayer(Sequence* sequence, var params);
	~SequenceBlockLayer();
	
	SequenceBlockManager blockManager;

	SequenceBlock* currentBlock;
	CriticalSection blockLock;

	WeakReference<Inspectable> currentBlockRef;
	
	virtual void getSnapTimes(Array<float>* arrayToFill) override;

	virtual void clearItem() override;


	void itemAdded(LayerBlock* item) override;
	void itemsAdded(Array<LayerBlock*> items) override;
	void itemRemoved(LayerBlock* item) override;
	void itemsRemoved(Array<LayerBlock*> items) override;

	void updateCurrentBlock();

	void updateCurrentSequenceTime();

	void onControllableFeedbackUpdateInternal(ControllableContainer *cc, Controllable * c) override;

	void selectAll(bool addToSelection = false) override;

	virtual var getJSONData(bool includeNonOverriden = false) override;
	virtual void loadJSONDataInternal(var data) override;

	virtual SequenceLayerPanel * getPanel() override;
	virtual SequenceLayerTimeline * getTimelineUI() override;
	
	void sequenceCurrentTimeChanged(Sequence *, float prevTime, bool evaluatedSkippedData) override;
	void sequencePlayStateChanged(Sequence *) override;
	//void sequencePlaySpeedChanged(Sequence*) override;


	static SequenceBlockLayer* create(Sequence* sequence, var params) { return new SequenceBlockLayer(sequence, params); }
	DECLARE_TYPE("Sequences");

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceBlockLayer)
	
};