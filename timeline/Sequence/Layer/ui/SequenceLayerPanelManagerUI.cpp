/*
  ==============================================================================

    SequenceLayerPanelManagerUI.cpp
    Created: 28 Oct 2016 8:15:41pm
    Author:  bkupe

  ==============================================================================
*/

SequenceLayerPanelManagerUI::SequenceLayerPanelManagerUI(SequenceLayerManager * _manager) :
	ManagerUI<SequenceLayerManager, SequenceLayer, SequenceLayerPanel>("SequenceLayers", _manager)
{
	viewport.setScrollBarsShown(false, false, true, false);
	animateItemOnAdd = false;
	addExistingItems();
}

SequenceLayerPanelManagerUI::~SequenceLayerPanelManagerUI()
{
}



SequenceLayerPanel * SequenceLayerPanelManagerUI::createUIForItem(SequenceLayer * layer)
{
	return layer->getPanel();
}

void SequenceLayerPanelManagerUI::addItemFromMenu(SequenceLayer* item, bool fromAddButton, Point<int> pos)
{
    ManagerUI::addItemFromMenu(item, fromAddButton, pos);
    item->addDefaultContent();
}
