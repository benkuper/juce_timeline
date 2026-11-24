/*
  ==============================================================================

    VideoLayerTimeline.cpp
    Created: 26 Sep 2026

  ==============================================================================
*/

VideoLayerTimeline::VideoLayerTimeline(VideoLayer* layer) :
	SequenceLayerTimeline(layer),
	videoLayer(layer)
{
	bgColor = VIDEO_COLOR.withSaturation(.2f).darker(1);

	cmMUI.reset(new VideoLayerClipManagerUI(this, &layer->clipManager));
	addAndMakeVisible(cmMUI.get());

	updateMiniModeUI();

	needle.toFront(false);
}

VideoLayerTimeline::~VideoLayerTimeline()
{
}

void VideoLayerTimeline::resized()
{
	cmMUI->setBounds(getLocalBounds());
}

void VideoLayerTimeline::updateContent()
{
	cmMUI->updateContent();
}

void VideoLayerTimeline::updateMiniModeUI()
{
	cmMUI->setMiniMode(item->miniMode->boolValue());
}

void VideoLayerTimeline::addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables)
{
	if (cmMUI == nullptr) return;
	cmMUI->addSelectableComponentsAndInspectables(selectables, inspectables);
}