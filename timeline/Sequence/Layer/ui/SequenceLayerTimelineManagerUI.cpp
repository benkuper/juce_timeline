#include "SequenceLayerTimelineManagerUI.h"
/*
  ==============================================================================

    SequenceLayerTimelineManagerUI.cpp
    Created: 20 Nov 2016 2:18:18pm
    Author:  Ben Kuper

  ==============================================================================
*/

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
		if (i->isVisible())
		{
			i->addSelectableComponentsAndInspectables(selectables, inspectables);
		}
	}

}

bool SequenceLayerTimelineManagerUI::isInterestedInFileDrag(const StringArray& files)
{
	for (int i = 0; i < files.size(); i++)
	{
		if (files[i].endsWith("mp3") || files[i].endsWith("wav") || files[i].endsWith("aiff")) return true;
	}

	return false;
}

void SequenceLayerTimelineManagerUI::filesDropped(const StringArray& files, int x, int y)
{
	for (int i = 0; i < files.size(); i++)
	{
		manager->fileDropped(files[i]);
	}
}
