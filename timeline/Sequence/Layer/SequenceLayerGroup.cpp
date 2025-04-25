#include "JuceHeader.h"

SequenceLayerGroup::SequenceLayerGroup(Sequence* s) :
	ItemBaseGroup<SequenceLayer>(new SequenceLayerManager(s), "Layer Group")
{
	jassert(s != nullptr);
}

SequenceLayerGroup::~SequenceLayerGroup()
{
}


