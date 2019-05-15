/*
  ==============================================================================

    SequenceTimelineNavigationUI.cpp
    Created: 20 Nov 2016 2:25:19pm
    Author:  Ben Kuper

  ==============================================================================
*/

SequenceTimelineNavigationUI::SequenceTimelineNavigationUI(Sequence * _sequence, SequenceTimelineSeeker * seeker, SequenceTimelineHeader * header) :
	sequence(_sequence),
	seeker(seeker),
	header(header)
{
	if (this->seeker == nullptr) this->seeker.reset(new SequenceTimelineSeeker(sequence));
	addAndMakeVisible(this->seeker.get());
	if(this->header == nullptr) this->header.reset(new SequenceTimelineHeader(sequence));
	addAndMakeVisible(this->header.get());
}

SequenceTimelineNavigationUI::~SequenceTimelineNavigationUI()
{
}

void SequenceTimelineNavigationUI::paint(Graphics &)
{
}

void SequenceTimelineNavigationUI::resized()
{
	Rectangle<int> r = getLocalBounds();
	
	//compensate for layerTimelineManagerUI margins
	r.removeFromLeft(4);
	r.removeFromRight(10);
	
	r.removeFromTop(4);
	seeker->setBounds(r.removeFromTop(15));
	r.reduce(0, 2);
	header->setBounds(r);
}
