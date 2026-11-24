/*
  ==============================================================================

    VideoLayerClip.cpp
    Created: 26 Sep 2026

  ==============================================================================
*/

#include "JuceHeader.h"
#include "VideoLayerClip.h"
#include "VideoFileHelpers.h"

VideoLayerClip::VideoLayerClip() :
	LayerBlock(getTypeString()),
	clipDuration(0)
{
	itemDataType = getTypeString();
	itemColor->setDefaultValue(VIDEO_COLOR.darker());

	// Persist the clip's child containers (the Transform group) in the project
	// file : without this, only the clip-level parameters are saved.
	saveAndLoadRecursiveData = true;

	filePath = new FileParameter("File Path", "File Path", "");
	addParameter(filePath);

	clipLength = addFloatParameter("Clip Length", "Length of the clip (in seconds)", 0);
	clipLength->defaultUI = FloatParameter::TIME;
	clipLength->setControllableFeedbackOnly(true);
	clipLength->isSavable = false;

	clipStartOffset = addFloatParameter("Clip Start Offset", "Offset at which the clip starts", 0, 0);
	clipStartOffset->defaultUI = FloatParameter::TIME;

	volume = addFloatParameter("Volume", "Volume multiplier", 1, 0, 2);

	fadeIn = addFloatParameter("Fade In", "Fade time at the start of the clip", 0, 0);
	fadeIn->defaultUI = FloatParameter::TIME;
	fadeOut = addFloatParameter("Fade Out", "Fade time at the end of the clip", 0, 0);
	fadeOut->defaultUI = FloatParameter::TIME;

	opacity = addFloatParameter("Opacity", "Opacity of the clip, 0 = fully transparent", 1, 0, 1);

	transform = new ControllableContainer("Transform");
	addChildControllableContainer(transform, true);

	width = transform->addFloatParameter("Width", "Horizontal scale, in percents", 100, 1, 10000);
	height = transform->addFloatParameter("Height", "Vertical scale, in percents", 100, 1, 10000);
	size = transform->addFloatParameter("Size", "Uniform scale, in percents", 100, 1, 10000);
	x = transform->addFloatParameter("X", "Horizontal offset, in percents of the view width", 0, -1000, 1000);
	y = transform->addFloatParameter("Y", "Vertical offset, in percents of the view height", 0, -1000, 1000);

	blendMode = addEnumParameter("Blend Mode", "How this clip blends with the clips below it in the composition");
	blendMode->addOption("Normal", (int) BlendMode::Normal);
	blendMode->addOption("Add", (int) BlendMode::Add);
	blendMode->addOption("Multiply", (int) BlendMode::Multiply);
	blendMode->addOption("Screen", (int) BlendMode::Screen);
	blendMode->addOption("Lighten", (int) BlendMode::Lighten);
	blendMode->addOption("Darken", (int) BlendMode::Darken);
	blendMode->addOption("Overlay", (int) BlendMode::Overlay);
	blendMode->addOption("Difference", (int) BlendMode::Difference);
	blendMode->addOption("Exclusion", (int) BlendMode::Exclusion);
}

VideoLayerClip::~VideoLayerClip()
{
	masterReference.clear();
}

void VideoLayerClip::onContainerParameterChangedInternal(Parameter* p)
{
	LayerBlock::onContainerParameterChangedInternal(p);

	if (p == filePath)
	{
		clearThumbnails();
		clipListeners.call(&ClipListener::clipSourceLoaded, this);
	}

	if (p == volume || p == fadeIn || p == fadeOut || p == opacity || p == width || p == height || p == size || p == x || p == y || p == blendMode)
	{
		clipListeners.call(&ClipListener::clipParamChanged, this);
	}
}

void VideoLayerClip::setCoreLength(float value, bool stretch, bool stickToCoreEnd)
{
	LayerBlock::setCoreLength(value, stretch, stickToCoreEnd);
}

void VideoLayerClip::setStartTime(float value, bool stretch, bool stickToCoreEnd)
{
	LayerBlock::setStartTime(value, stretch, stickToCoreEnd);
}

float VideoLayerClip::getManualFadeFactor(double timelineTime)
{
	const double fromStart = timelineTime - time->doubleValue();
	const double toEnd = getEndTime() - timelineTime;
	float factor = 1.0f;

	if (fadeIn->doubleValue() > 0.0 && fromStart < fadeIn->doubleValue())
	{
		const float f = juce::jlimit(0.0f, 1.0f, (float) (fromStart / fadeIn->doubleValue()));
		factor *= f * f;
	}

	if (fadeOut->doubleValue() > 0.0 && toEnd < fadeOut->doubleValue())
	{
		const float f = juce::jlimit(0.0f, 1.0f, (float) (toEnd / fadeOut->doubleValue()));
		factor *= f * f;
	}

	return factor;
}

void VideoLayerClip::cacheThumbnail(double sourceTime, const juce::Image& frame)
{
	if (!frame.isValid()) return;

	const double spacing = juce::jmax(0.35, clipDuration > 0.0 ? clipDuration / 36.0 : 0.35);
	const juce::ScopedLock lock(thumbnailLock);

	for (auto& sample : thumbnailSamples)
		if (std::abs(sample.sourceTime - sourceTime) < spacing) return;

	const int targetWidth = juce::jmin(240, frame.getWidth());
	const int targetHeight = juce::jmax(1, juce::roundToInt(targetWidth * frame.getHeight() / (float) juce::jmax(1, frame.getWidth())));
	ThumbnailSample sample;
	sample.sourceTime = sourceTime;
	sample.image = frame.rescaled(targetWidth, targetHeight, juce::Graphics::lowResamplingQuality);

	int insertAt = 0;
	while (insertAt < thumbnailSamples.size() && thumbnailSamples.getReference(insertAt).sourceTime < sourceTime) ++insertAt;
	thumbnailSamples.insert(insertAt, sample);

	while (thumbnailSamples.size() > 48)
		thumbnailSamples.remove((thumbnailSamples.size() > 2) ? 1 : 0);

	clipListeners.call(&ClipListener::clipThumbnailChanged, this);
}

bool VideoLayerClip::needsThumbnail(double sourceTime) const
{
	const double spacing = juce::jmax(0.35, clipDuration > 0.0 ? clipDuration / 36.0 : 0.35);
	const juce::ScopedLock lock(thumbnailLock);
	for (auto& sample : thumbnailSamples)
		if (std::abs(sample.sourceTime - sourceTime) < spacing) return false;
	return true;
}

juce::Image VideoLayerClip::getThumbnailForTime(double sourceTime) const
{
	const juce::ScopedLock lock(thumbnailLock);
	if (thumbnailSamples.isEmpty()) return {};

	int best = 0;
	double bestDistance = std::abs(thumbnailSamples.getReference(0).sourceTime - sourceTime);
	for (int i = 1; i < thumbnailSamples.size(); ++i)
	{
		const double distance = std::abs(thumbnailSamples.getReference(i).sourceTime - sourceTime);
		if (distance < bestDistance)
		{
			best = i;
			bestDistance = distance;
		}
	}

	return thumbnailSamples.getReference(best).image;
}

void VideoLayerClip::clearThumbnails()
{
	const juce::ScopedLock lock(thumbnailLock);
	thumbnailSamples.clear();
}
