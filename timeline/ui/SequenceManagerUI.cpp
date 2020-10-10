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

	playAllUI.reset(manager->playAllTrigger->createImageUI(ImageCache::getFromMemory(TimelineBinaryData::play_png, TimelineBinaryData::play_pngSize)));
	stopAllUI.reset(manager->stopAllTrigger->createImageUI(ImageCache::getFromMemory(TimelineBinaryData::stop_png, TimelineBinaryData::stop_pngSize)));
	addAndMakeVisible(playAllUI.get());
	addAndMakeVisible(stopAllUI.get());

	setShowSearchBar(true);
	addExistingItems();
}

SequenceManagerUI::~SequenceManagerUI()
{
}

void SequenceManagerUI::resizedInternalHeader(Rectangle<int>& r)
{
	BaseManagerShapeShifterUI::resizedInternalHeader(r);
	playAllUI->setBounds(r.removeFromLeft(r.getHeight()).reduced(2));
	stopAllUI->setBounds(r.removeFromLeft(r.getHeight()).reduced(2));
}

void SequenceManagerUI::mouseDoubleClick(const MouseEvent & e)
{
	BaseManagerShapeShifterUI::mouseDoubleClick(e);
	manager->addItem();
}
