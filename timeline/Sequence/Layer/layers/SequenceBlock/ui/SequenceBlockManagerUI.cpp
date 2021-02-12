/*
  ==============================================================================

    SequenceBlockManagerUI.cpp
    Created: 8 Feb 2017 12:20:16pm
    Author:  Ben

  ==============================================================================
*/


SequenceBlockManagerUI::SequenceBlockManagerUI(SequenceBlockLayerTimeline * _timeline, SequenceBlockManager * manager) :
	LayerBlockManagerUI(_timeline, manager)
{
	addExistingItems();
}

SequenceBlockManagerUI::~SequenceBlockManagerUI()
{
}


LayerBlockUI* SequenceBlockManagerUI::createUIForItem(LayerBlock* item)
{
	SequenceBlockUI * ui =  new SequenceBlockUI(dynamic_cast<SequenceBlock *>(item));
	return ui;
}