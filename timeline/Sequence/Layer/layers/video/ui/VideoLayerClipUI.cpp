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
}

VideoLayerClipUI::~VideoLayerClipUI()
{
}

void VideoLayerClipUI::paint(Graphics& g)
{
	LayerBlockUI::paint(g);

	Rectangle<int> b = getCoreBounds();

	//film perforations
	g.setColour(Colours::black.withAlpha(.35f));

	int size = 4;
	int gap = 6;
	for (int x = b.getX() + 2; x < b.getRight() - 2; x += size + gap)
	{
		g.fillRect(x, b.getY() + 2, size, 3);
		g.fillRect(x, b.getBottom() - 5, size, 3);
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