/*
  ==============================================================================

    AudioLayerTimeline.h
    Created: 20 Nov 2016 3:09:01pm
    Author:  Ben Kuper

  ==============================================================================
*/

#ifndef AUDIOLAYERTIMELINE_H_INCLUDED
#define AUDIOLAYERTIMELINE_H_INCLUDED

class AudioLayerTimeline :
	public SequenceLayerTimeline
{
public:
	AudioLayerTimeline(AudioLayer * layer);
	~AudioLayerTimeline();

	AudioLayer * audioLayer;
	ScopedPointer<AudioLayerClipManagerUI> cmMUI;

	void resized() override;
	void updateContent() override;
};



#endif  // AUDIOLAYERTIMELINE_H_INCLUDED
