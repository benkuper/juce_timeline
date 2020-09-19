/*
  ==============================================================================

	AudioLayerClipUI.cpp
	Created: 8 Feb 2017 12:20:09pm
	Author:  Ben

  ==============================================================================
*/

AudioLayerClipUI::AudioLayerClipUI(AudioLayerClip* _clip) :
	LayerBlockUI(_clip),
	thumbnailCache(100000),
	thumbnail(50, _clip->formatManager, thumbnailCache),
	clip(_clip)
{
	dragAndDropEnabled = false;

	clip->addAsyncClipListener(this);

	bgColor = clip->isCurrent ? AUDIO_COLOR.brighter() : BG_COLOR.brighter(.1f);

#if JUCE_WINDOWS
	if (clip->filePath->stringValue().startsWithChar('/')) return;
#endif

	thumbnail.setSource(new FileInputSource(clip->filePath->getFile()));

	repaint();
}

AudioLayerClipUI::~AudioLayerClipUI()
{
	if (!inspectable.wasObjectDeleted()) clip->removeAsyncClipListener(this);
}

void AudioLayerClipUI::paint(Graphics& g)
{
	LayerBlockUI::paint(g);

	if (clip->filePath->stringValue().isEmpty()) return;
	g.setColour(Colours::white.withAlpha(.5f));
	if (clip->isLoading)
	{
		g.setFont(20);
		g.drawText("Loading...", getLocalBounds(), Justification::centred);

	}
	else
	{
		thumbnail.drawChannels(g, getCoreBounds(), clip->clipStartOffset->floatValue(), clip->clipStartOffset->floatValue() + clip->coreLength->floatValue() / clip->stretchFactor->floatValue(), clip->volume->floatValue());
	}

	if (clip->fadeIn->floatValue() > 0)
	{
		g.setColour(YELLOW_COLOR.withAlpha(.2f));
		int fadeInWidth = clip->fadeIn->floatValue() * getCoreWidth() / clip->coreLength->floatValue(); 
		g.fillRect(getCoreBounds().removeFromLeft(fadeInWidth));
	}
	
	if (clip->fadeOut->floatValue() > 0)
	{
		g.setColour(YELLOW_COLOR.withAlpha(.2f));
		int fadeOutWidth = clip->fadeOut->floatValue() * getCoreWidth() / clip->coreLength->floatValue();
		g.fillRect(getCoreBounds().removeFromRight(fadeOutWidth));
	}
}

void AudioLayerClipUI::resizedBlockInternal()
{
	LayerBlockUI::resizedBlockInternal();
}


void AudioLayerClipUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	LayerBlockUI::controllableFeedbackUpdateInternal(c);

	if (c == item->time || c == item->coreLength || c == clip->volume || c == clip->fadeIn || c == clip->fadeOut)
	{
		repaint();
	}
}

void AudioLayerClipUI::newMessage(const AudioLayerClip::ClipEvent& e)
{
	switch (e.type)
	{
	case AudioLayerClip::ClipEvent::CLIP_IS_CURRENT_CHANGED:
		bgColor = clip->isCurrent ? AUDIO_COLOR.brighter() : BG_COLOR.brighter(.1f);
		repaint();
		break;

	case AudioLayerClip::ClipEvent::SOURCE_LOAD_START:
		thumbnail.setSource(nullptr);
		repaint();
		break;

	case AudioLayerClip::ClipEvent::SOURCE_LOAD_END:
		thumbnail.setSource(new FileInputSource(clip->filePath->getFile()));
		repaint();
		break;

	}

}