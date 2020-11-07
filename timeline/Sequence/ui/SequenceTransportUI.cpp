/*
  ==============================================================================

    SequenceTransportUI.cpp
    Created: 20 Nov 2016 2:24:35pm
    Author:  Ben Kuper

  ==============================================================================
*/


SequenceTransportUI::SequenceTransportUI(Sequence* _sequence) :
	sequence(_sequence),
	sequenceRef(_sequence),
	timeLabel(_sequence->currentTime),
	timeStepLabel(_sequence->currentTime),
    totalTimeLabel(_sequence->totalTime)
{
	timeLabel.maxFontHeight = 20;
	totalTimeLabel.maxFontHeight = 14;
	totalTimeLabel.useCustomTextColor = true;
	totalTimeLabel.customTextColor = TEXT_COLOR.darker();
	totalTimeLabel.updateUIParams();

	timeStepLabel.maxFontHeight = 12;
	timeStepLabel.useCustomTextColor = true;
	timeStepLabel.customTextColor = TEXT_COLOR.darker(.2f);
	timeStepLabel.setShowStepsMode(true);
	timeStepLabel.updateUIParams();

	addAndMakeVisible(&timeLabel);
	addAndMakeVisible(&totalTimeLabel);
	addAndMakeVisible(&timeStepLabel);
	 
	togglePlayUI.reset(sequence->togglePlayTrigger->createImageUI(ImageCache::getFromMemory(TimelineBinaryData::play_png, TimelineBinaryData::play_pngSize)));
	stopUI.reset(sequence->stopTrigger->createImageUI(ImageCache::getFromMemory(TimelineBinaryData::stop_png, TimelineBinaryData::stop_pngSize)));
	nextCueUI.reset(sequence->nextCue->createImageUI(ImageCache::getFromMemory(TimelineBinaryData::nextcue_png, TimelineBinaryData::nextcue_pngSize)));
	prevCueUI.reset(sequence->prevCue->createImageUI(ImageCache::getFromMemory(TimelineBinaryData::prevcue_png, TimelineBinaryData::prevcue_pngSize)));
	loopUI.reset(sequence->loopParam->createImageToggle(AssetManager::getInstance()->getToggleBTImage(ImageCache::getFromMemory(TimelineBinaryData::loop_png, TimelineBinaryData::loop_pngSize))));

	addAndMakeVisible(togglePlayUI.get());
	addAndMakeVisible(stopUI.get());
	addAndMakeVisible(nextCueUI.get());
	addAndMakeVisible(prevCueUI.get());
	addAndMakeVisible(loopUI.get());

	sequence->addAsyncCoalescedSequenceListener(this);
}

SequenceTransportUI::~SequenceTransportUI()
{
	if (!sequenceRef.wasObjectDeleted()) sequence->removeAsyncSequenceListener(this);

	togglePlayUI = nullptr;
	stopUI = nullptr;
	nextCueUI = nullptr;
	prevCueUI = nullptr;
	loopUI = nullptr;
}

void SequenceTransportUI::resized()
{
	Rectangle<int> r = getLocalBounds().reduced(2);
	Rectangle<int> tr = r.removeFromTop(30);
	timeStepLabel.setBounds(tr.removeFromRight(40));
	tr.removeFromLeft(40);
	timeLabel.setBounds(tr);
	r.removeFromTop(2);
	
	Rectangle<int> pr = r.removeFromBottom(24);
	togglePlayUI->setBounds(pr.removeFromLeft(pr.getHeight()).reduced(2,0));
	stopUI->setBounds(pr.removeFromLeft(pr.getHeight()).reduced(4, 2));
	prevCueUI->setBounds(pr.removeFromLeft(pr.getHeight()).reduced(2));
	nextCueUI->setBounds(pr.removeFromLeft(pr.getHeight()).reduced(2));
	loopUI->setBounds(pr.removeFromLeft(30).reduced(4));
	totalTimeLabel.setBounds(pr.removeFromRight(100));
}

void SequenceTransportUI::newMessage(const Sequence::SequenceEvent& e)
{
	if (e.type == e.PLAY_STATE_CHANGED)
	{
		if (!sequenceRef.wasObjectDeleted())
		{
			togglePlayUI->forceDrawTriggering = sequence->isPlaying->boolValue();
			togglePlayUI->repaint();
		}
	}
}

