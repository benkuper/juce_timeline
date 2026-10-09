/*
  ==============================================================================

    TriggerLayerPanel.cpp
    Created: 20 Nov 2016 3:07:50pm
    Author:  Ben Kuper

  ==============================================================================
*/

TriggerLayerPanel::TriggerLayerPanel(TriggerLayer * layer) :
	SequenceLayerPanel(layer),
	triggerLayer(layer)
{
	lockAllBT.reset(layer->lockAll->createButtonUI());
	unlockAllBT.reset(layer->unlockAll->createButtonUI());
	addAndMakeVisible(lockAllBT.get());
	addAndMakeVisible(unlockAllBT.get());

	contentComponents.add(lockAllBT.get());
	contentComponents.add(unlockAllBT.get());
	triggerAllTrueBT.reset(layer->triggerAllTrue->createButtonUI());
	triggerAllFalseBT.reset(layer->triggerAllFalse->createButtonUI());
	addAndMakeVisible(triggerAllTrueBT.get());
	addAndMakeVisible(triggerAllFalseBT.get());
	contentComponents.add(triggerAllTrueBT.get());
	contentComponents.add(triggerAllFalseBT.get());


}

TriggerLayerPanel::~TriggerLayerPanel()
{
}

void TriggerLayerPanel::resizedInternalContent(Rectangle<int>& r)
{
	SequenceLayerPanel::resizedInternalContent(r);

	Rectangle<int> btr = r.removeFromTop(12);

	lockAllBT->setBounds(btr.removeFromLeft(60));
	btr.removeFromLeft(10);
	unlockAllBT->setBounds(btr.removeFromLeft(60));
	Rectangle<int> actions = r.removeFromTop(16);
	triggerAllTrueBT->setBounds(actions.removeFromLeft(95));
	actions.removeFromLeft(4);
	triggerAllFalseBT->setBounds(actions.removeFromLeft(95));
}
