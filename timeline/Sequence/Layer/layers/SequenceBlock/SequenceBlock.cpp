/*
  ==============================================================================

	AudioLayerClip.cpp
	Created: 8 Feb 2017 12:20:02pm
	Author:  Ben

  ==============================================================================
*/


SequenceBlock::SequenceBlock(SequenceManager* manager) :
	LayerBlock(getTypeString()),
	targetSequence(nullptr)
{
	jassert(manager != nullptr);

	target = addTargetParameter("Sequence", "The target Sequence to control", manager);
	target->targetType = TargetParameter::CONTAINER;
	target->maxDefaultSearchLevel = 0;

	sequenceStartOffset = addFloatParameter("Sequence Start Offset", "Offset at which the sequence will start", 0, 0);
	sequenceStartOffset->defaultUI = FloatParameter::TIME;
}

SequenceBlock::~SequenceBlock()
{
	setTargetSequence(nullptr);

}

void SequenceBlock::setTargetSequence(Sequence* sequence)
{
	if (sequence == targetSequence) return;


	targetSequence = sequence;
	sequenceRef = sequence;

	if (targetSequence != nullptr)
	{
		if (!isCurrentlyLoadingData && !coreLength->isOverriden) setCoreLength(targetSequence->totalTime->floatValue(), false);
	}
}

Sequence* SequenceBlock::getTargetSequence()
{
	return !sequenceRef.wasObjectDeleted() ? targetSequence : nullptr;
}

void SequenceBlock::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == target) setTargetSequence(((Sequence*)target->targetContainer.get()));
}
