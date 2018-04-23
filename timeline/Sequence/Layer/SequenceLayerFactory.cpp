/*
  ==============================================================================

    SequenceLayerFactory.cpp
    Created: 20 Nov 2016 3:09:30pm
    Author:  Ben Kuper

  ==============================================================================
*/

#include "SequenceLayerFactory.h"


SequenceLayerFactory::SequenceLayerFactory() {
}

void SequenceLayerFactory::buildPopupMenu()
{
	int id = 1;
	for (auto &d : layerDefs)
	{
		menu.addItem(id, d->type);
		id++;
	}

}

SequenceLayerDefinition * SequenceLayerDefinition::createDef(const String & _type, std::function<SequenceLayer*(Sequence*, var)> createFunc)
{
	SequenceLayerDefinition * s = new SequenceLayerDefinition(_type, createFunc);
	return s;
}

SequenceLayerDefinition * SequenceLayerDefinition::addParam(const String & paramName, var value)
{
	params.getDynamicObject()->setProperty(paramName, value);
	return this;
}
