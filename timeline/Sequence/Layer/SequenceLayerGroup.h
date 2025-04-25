#pragma once

class SequenceLayerGroup;

class SequenceLayerGroup :
	public ItemBaseGroup<SequenceLayer>
{
public:
	SequenceLayerGroup(Sequence* s = nullptr);
	~SequenceLayerGroup();
};