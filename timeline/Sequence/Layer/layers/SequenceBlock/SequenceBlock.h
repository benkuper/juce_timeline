/*
  ==============================================================================

    SequenceBlock.h
    Created: 8 Feb 2017 12:20:02pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class SequenceManager;

class SequenceBlock :
	public LayerBlock
{
public:
	SequenceBlock(SequenceManager * manager);
	virtual ~SequenceBlock();

	TargetParameter * target;
	FloatParameter* sequenceStartOffset;

	Sequence* targetSequence;
	WeakReference<Inspectable> sequenceRef;

	void setTargetSequence(Sequence* sequence);
	Sequence* getTargetSequence();


	void onContainerParameterChangedInternal(Parameter *) override;
	DECLARE_TYPE("SequenceBlock");
};


