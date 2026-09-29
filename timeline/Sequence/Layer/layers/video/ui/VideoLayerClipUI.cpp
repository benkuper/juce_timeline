/*
  ==============================================================================

    VideoLayerClipUI.cpp
    Created: 26 Sep 2026

  ==============================================================================
*/

#include "JuceHeader.h"

VideoLayerClipUI::VideoLayerClipUI(VideoLayerClip* _clip) :
	LayerBlockUI(_clip),
	clip(_clip)
{
	dragAndDropEnabled = false;
	bgColor = clip->isActive->boolValue() ? VIDEO_COLOR.brighter() : VIDEO_COLOR.darker();
	clip->addClipListener(this);
}

VideoLayerClipUI::~VideoLayerClipUI()
{
	if (!inspectable.wasObjectDeleted()) clip->removeClipListener(this);
}

void VideoLayerClipUI::paint(Graphics& g)
{
	LayerBlockUI::paint(g);

	Rectangle<int> b = getCoreBounds();
	Rectangle<int> previewBounds = b.reduced(2).withTrimmedTop(5).withTrimmedBottom(5);

	if (!clip->filePath->stringValue().isEmpty() && !previewBounds.isEmpty())
	{
		const int cellWidth = juce::jmax(28, juce::roundToInt(previewBounds.getHeight() * 1.7f));
		const int cells = juce::jlimit(1, 32, (previewBounds.getWidth() + cellWidth - 1) / cellWidth);
		for (int i = 0; i < cells; ++i)
		{
			const int left = previewBounds.getX() + i * previewBounds.getWidth() / cells;
			const int right = previewBounds.getX() + (i + 1) * previewBounds.getWidth() / cells;
			const double relativeTime = viewStart + (viewCoreEnd - viewStart) * (i + 0.5) / cells;
			Image thumbnail = clip->getThumbnailForTime(clip->clipStartOffset->doubleValue() + relativeTime);
			if (thumbnail.isValid())
				g.drawImageWithin(thumbnail, left, previewBounds.getY(), juce::jmax(1, right - left), previewBounds.getHeight(), RectanglePlacement::fillDestination);
		}

		g.setColour(Colours::black.withAlpha(0.28f));
		g.fillRect(previewBounds.removeFromBottom(14));
	}

	//film perforations
	g.setColour(Colours::black.withAlpha(.35f));

	int size = 4;
	int gap = 6;
	for (int x = b.getX() + 2; x < b.getRight() - 2; x += size + gap)
	{
		g.fillRect(x, b.getY() + 2, size, 3);
		g.fillRect(x, b.getBottom() - 5, size, 3);
	}

	if (clip->fadeIn->floatValue() > 0.0f && clip->coreLength->floatValue() > 0.0f)
	{
		const int fadeWidth = juce::roundToInt(clip->fadeIn->floatValue() * b.getWidth() / clip->coreLength->floatValue());
		Path p;
		p.startNewSubPath((float) b.getX(), (float) b.getY());
		p.lineTo((float) (b.getX() + fadeWidth), (float) b.getY());
		p.lineTo((float) b.getX(), (float) b.getBottom());
		p.closeSubPath();
		g.setColour(YELLOW_COLOR.withAlpha(.25f));
		g.fillPath(p);
	}

	if (clip->fadeOut->floatValue() > 0.0f && clip->coreLength->floatValue() > 0.0f)
	{
		const int fadeWidth = juce::roundToInt(clip->fadeOut->floatValue() * b.getWidth() / clip->coreLength->floatValue());
		Path p;
		p.startNewSubPath((float) b.getRight(), (float) b.getY());
		p.lineTo((float) (b.getRight() - fadeWidth), (float) b.getY());
		p.lineTo((float) b.getRight(), (float) b.getBottom());
		p.closeSubPath();
		g.setColour(YELLOW_COLOR.withAlpha(.25f));
		g.fillPath(p);
	}

	if (clip->filePath->stringValue().isEmpty())
	{
		g.setColour(Colours::white.withAlpha(.4f));
		g.setFont(10);
		g.drawText("Double-click to load a video", b, Justification::centred);
		return;
	}

	//file name + info
	g.setColour(Colours::white.withAlpha(.6f));
	g.setFont(10);

	Rectangle<int> textBounds = b.withTrimmedTop(7).withTrimmedBottom(7).withTrimmedLeft(6).withTrimmedRight(6);

	String name = File(clip->filePath->stringValue()).getFileName();
	g.drawText(name, textBounds, Justification::centredLeft);

	String timeInfo = timeValueToString(clip->time->floatValue()) + "  /  " + timeValueToString(clip->getTotalLength());
	g.setColour(Colours::white.withAlpha(.3f));
	g.drawText(timeInfo, textBounds, Justification::centredRight);

	if (clip->isActive->boolValue())
	{
		g.setColour(RED_COLOR.withAlpha(.8f));
		g.fillRect(b.removeFromLeft(2));
	}
}

void VideoLayerClipUI::resizedBlockInternal()
{
}

void VideoLayerClipUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	LayerBlockUI::controllableFeedbackUpdateInternal(c);
	if (c == clip->time || c == clip->coreLength || c == clip->clipStartOffset || c == clip->fadeIn || c == clip->fadeOut || c == clip->isActive)
		shouldRepaint = true;
}

void VideoLayerClipUI::clipSourceLoaded(VideoLayerClip*)
{
	shouldRepaint = true;
}

void VideoLayerClipUI::clipThumbnailChanged(VideoLayerClip*)
{
	shouldRepaint = true;
}
