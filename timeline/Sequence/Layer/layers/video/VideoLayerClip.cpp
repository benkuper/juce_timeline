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
		clipListeners.call(&ClipListener::clipSourceLoaded, this);
	}

	if (p == volume || p == opacity || p == width || p == height || p == size || p == x || p == y || p == blendMode)
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