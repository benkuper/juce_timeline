/*
  ==============================================================================

    SequenceUI.cpp
    Created: 28 Oct 2016 8:13:34pm
    Author:  bkupe

  ==============================================================================
*/

SequenceUI::SequenceUI(Sequence * sequence) :
	BaseItemUI<Sequence>(sequence)
{
	minContentHeight = 20; //To fix : need to pass in constructor of BaseItemUI

	togglePlayUI.reset(item->isPlaying->createImageToggle(AssetManager::getInstance()->getToggleBTImage(ImageCache::getFromMemory(TimelineBinaryData::play_png, TimelineBinaryData::play_pngSize))));
	stopUI.reset(item->stopTrigger->createImageUI(ImageCache::getFromMemory(TimelineBinaryData::stop_png, TimelineBinaryData::stop_pngSize)));
	addAndMakeVisible(togglePlayUI.get());
	addAndMakeVisible(stopUI.get());


	timeUI.reset(item->currentTime->createSlider());
	timeUI->showLabel = false;
	timeUI->showValue = false;
	timeUI->useCustomBGColor = true;
	timeUI->useCustomFGColor = true;
	timeUI->customBGColor = BG_COLOR.darker(.1f);
	timeUI->customFGColor = FEEDBACK_COLOR.withSaturation(.3f);

	addAndMakeVisible(timeUI.get());

	bgColor = item->isBeingEdited ? BLUE_COLOR.darker() : BG_COLOR.brighter(.1f); 
	
	item->addAsyncContainerListener(this);
	item->addAsyncSequenceListener(this);
}

SequenceUI::~SequenceUI()
{
	item->removeAsyncContainerListener(this);
	if (!Engine::mainEngine->isClearing && !inspectable.wasObjectDeleted()) item->removeAsyncSequenceListener(this);
}

void SequenceUI::resizedInternalHeader(Rectangle<int>& r)
{
	stopUI->setBounds(r.removeFromRight(r.getHeight()).reduced(2));
	r.removeFromRight(2);
	togglePlayUI->setBounds(r.removeFromRight(r.getHeight()).reduced(2));
	r.removeFromRight(2);


	BaseItemUI::resizedInternalHeader(r);

}

void SequenceUI::resizedInternalContent(Rectangle<int>& r)
{
	r.setHeight(8);
	timeUI->setBounds(r);
}

void SequenceUI::controllableFeedbackUpdateInternal(Controllable * c)
{
	if (c == item->isPlaying)
	{
		timeUI->customFGColor = item->isPlaying->boolValue() ? Colour(252,212,98) : FEEDBACK_COLOR.withSaturation(.3f);
		timeUI->repaint();
	}
}

void SequenceUI::newMessage(const Sequence::SequenceEvent & e)
{
	switch (e.type)
	{
	case Sequence::SequenceEvent::EDITING_STATE_CHANGED:
	{
		bgColor = item->isBeingEdited ? BLUE_COLOR.darker() : BG_COLOR.brighter(.1f);
		repaint();
	}
	break;
	}
}
