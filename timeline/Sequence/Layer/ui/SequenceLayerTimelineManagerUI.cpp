/*
  ==============================================================================

    SequenceLayerTimelineManagerUI.cpp
    Created: 20 Nov 2016 2:18:18pm
    Author:  Ben Kuper

  ==============================================================================
*/

#include "JuceHeader.h"

SequenceLayerTimelineManagerUI::SequenceLayerTimelineManagerUI(SequenceLayerManager * _manager) :
	BaseManagerUI<SequenceLayerManager, SequenceLayer, SequenceLayerTimeline>("Layers", _manager)
{
	addItemBT->setVisible(false);
	animateItemOnAdd = false;
	addExistingItems();
}

SequenceLayerTimelineManagerUI::~SequenceLayerTimelineManagerUI()
{
}

SequenceLayerTimeline * SequenceLayerTimelineManagerUI::createUIForItem(SequenceLayer * layer)
{
	return layer->getTimelineUI();
}

void SequenceLayerTimelineManagerUI::resized()
{
	BaseManagerUI::resized();

}

void SequenceLayerTimelineManagerUI::addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables)
{
	for (auto& i : itemsUI)
	{
		if (i->isVisible() && !i->item->isUILocked->boolValue())
		{
			i->addSelectableComponentsAndInspectables(selectables, inspectables);
		}
	}

}

bool SequenceLayerTimelineManagerUI::isInterestedInFileDrag(const StringArray& files)
{
	for (int i = 0; i < files.size(); ++i)
	{
		if (files[i].endsWith("mp3") || files[i].endsWith("wav") || files[i].endsWith("aiff")) return true;

#if JUCE_WINDOWS
		if (files[i].endsWith("mp4") || files[i].endsWith("mov") || files[i].endsWith("avi") || files[i].endsWith("mkv") || files[i].endsWith("wmv") || files[i].endsWith("webm") || files[i].endsWith("m4v")) return true;
#endif
	}

	return false;
}

void SequenceLayerTimelineManagerUI::filesDropped(const StringArray& files, int x, int y)
{
	for (int i = 0; i < files.size(); ++i)
	{
		manager->fileDropped(files[i]);
	}
}
