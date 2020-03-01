/*
  ==============================================================================

    SequenceManagerUI.cpp
    Created: 28 Oct 2016 8:13:29pm
    Author:  bkupe

  ==============================================================================
*/

SequenceManagerUI::SequenceManagerUI(const String &contentName, SequenceManager * _manager) :
	BaseManagerShapeShifterUI<SequenceManager, Sequence, SequenceUI>(contentName, _manager)
{
	addItemText = "Add Sequence";
	noItemText = "Add timed animation and control by creating a sequence";

	stopAllUI.reset(manager->stopAllTrigger->createButtonUI());
	addAndMakeVisible(stopAllUI.get());

	addExistingItems();
}

SequenceManagerUI::~SequenceManagerUI()
{
}

void SequenceManagerUI::resizedInternalHeader(Rectangle<int>& r)
{
	BaseManagerShapeShifterUI::resizedInternalHeader(r);
	stopAllUI->setBounds(r.removeFromLeft(80).reduced(2));
}

void SequenceManagerUI::mouseDoubleClick(const MouseEvent & e)
{
	BaseManagerShapeShifterUI::mouseDoubleClick(e);
	manager->addItem();
}
