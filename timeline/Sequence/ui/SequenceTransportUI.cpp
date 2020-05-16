/*
  ==============================================================================

    SequenceTransportUI.cpp
    Created: 20 Nov 2016 2:24:35pm
    Author:  Ben Kuper

  ==============================================================================
*/


SequenceTransportUI::SequenceTransportUI(Sequence* _sequence) :
	sequence(_sequence),
	timeLabel(_sequence->currentTime),
	totalTimeLabel(_sequence->totalTime),
	timeStepLabel(_sequence->currentTime)
{
	timeLabel.maxFontHeight = 16;
	timeLabel.maxFontHeight = 14;
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
	sequence->addSequenceListener(this);
	 
	
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
}

SequenceTransportUI::~SequenceTransportUI()
{
	togglePlayUI = nullptr;
	stopUI = nullptr;
	nextCueUI = nullptr;
	prevCueUI = nullptr;
	loopUI = nullptr;

	sequence->removeSequenceListener(this);
}

void SequenceTransportUI::paint(Graphics &g)
{
	/*
	if (sequence->masterAudioModule != nullptr)
	{
		Rectangle<int> r = getLocalBounds().reduced(2).removeFromTop(timeLabel.getHeight()).withLeft(timeLabel.getRight() + 5);
		g.setColour(sequence->timeIsDrivenByAudio()?AUDIO_COLOR.brighter(.7f):HIGHLIGHT_COLOR);
		g.drawFittedText(sequence->masterAudioModule->niceName, r.reduced(2), Justification::centred, 1);
	}
	*/
}

void SequenceTransportUI::resized()
{
	Rectangle<int> r = getLocalBounds().reduced(2);
	Rectangle<int> tr = r.removeFromTop(20);
	timeLabel.setBounds(tr.removeFromLeft(110));
	timeStepLabel.setBounds(tr.removeFromLeft(50));
	totalTimeLabel.setBounds(tr.removeFromRight(100));
	r.removeFromTop(2);
	
	
	Rectangle<int> pr = r.removeFromTop(24);
	togglePlayUI->setBounds(pr.removeFromLeft(pr.getHeight()).reduced(4));
	stopUI->setBounds(pr.removeFromLeft(pr.getHeight()).reduced(4));
	prevCueUI->setBounds(pr.removeFromLeft(pr.getHeight()).reduced(4));
	nextCueUI->setBounds(pr.removeFromLeft(pr.getHeight()).reduced(4));
	loopUI->setBounds(pr.removeFromLeft(30).reduced(4));

}

void SequenceTransportUI::sequenceMasterAudioModuleChanged(Sequence *)
{
	repaint();
}
