/*
  ==============================================================================

    SequenceLayerTimelineManagerUI.h
    Created: 20 Nov 2016 2:18:18pm
    Author:  Ben Kuper

  ==============================================================================
*/

#pragma once

class SequenceLayerTimelineManagerUI :
	public ManagerUI<SequenceLayerManager, SequenceLayer, SequenceLayerTimeline>,
	public FileDragAndDropTarget
{
public:

	SequenceLayerTimelineManagerUI(SequenceLayerManager *_manager);
	~SequenceLayerTimelineManagerUI();

	SequenceLayerTimeline * createUIForItem(SequenceLayer * layer) override;

	void resized() override;

	void addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables) override;

	virtual bool isInterestedInFileDrag(const StringArray& files) override;
	virtual void filesDropped(const StringArray& files, int x, int y) override;
};