/*
  ==============================================================================

    SequenceLayer.cpp
    Created: 28 Oct 2016 8:15:05pm
    Author:  bkupe

  ==============================================================================
*/

SequenceLayer::SequenceLayer(Sequence * _sequence, const String &name) :
	BaseItem(name,true),
	sequence(_sequence)
{
	jassert(sequence != nullptr);


	uiHeight = addIntParameter("UI Height", "Height of the layer in the editor", 60);
	uiHeight->hideInEditor = true;

	setHasCustomColor(true);
	itemColor->setDefaultValue(BG_COLOR.brighter(.2f));

	sequence = _sequence;
	sequence->addSequenceListener(this);

}

SequenceLayer::~SequenceLayer()
{
	if (!sequence->isClearing)
		sequence->removeSequenceListener(this);
}

Array<Inspectable*> SequenceLayer::selectAllItemsBetween(float start, float end)
{
	if (isUILocked->boolValue() || !enabled->boolValue()) return Array<Inspectable *>();
	return selectAllItemsBetweenInternal(start, end);
}

Array<Inspectable*> SequenceLayer::selectAllItemsBetweenInternal(float start, float end)
{
	return Array<Inspectable*>();
}

Array<UndoableAction*> SequenceLayer::getRemoveAllItemsBetween(float start, float end)
{
	if (isUILocked->boolValue() || !enabled->boolValue()) return Array<UndoableAction*>();
	return getRemoveAllItemsBetweenInternal(start, end);
}

Array<UndoableAction*> SequenceLayer::getRemoveAllItemsBetweenInternal(float start, float end)
{
	return Array<UndoableAction*>();
}

Array<UndoableAction*> SequenceLayer::getInsertTimespan(float start, float length)
{
	if (isUILocked->boolValue() || !enabled->boolValue()) return Array<UndoableAction*>();
	return getInsertTimespanInternal(start, length);
}

Array<UndoableAction*> SequenceLayer::getInsertTimespanInternal(float start, float length)
{
	return Array<UndoableAction*>();
}

Array<UndoableAction*> SequenceLayer::getRemoveTimespan(float start, float end)
{
	if (isUILocked->boolValue() || !enabled->boolValue()) return Array<UndoableAction*>();
	return getRemoveTimespanInternal(start, end);
}

Array<UndoableAction*> SequenceLayer::getRemoveTimespanInternal(float start, float end)
{
	return Array<UndoableAction*>();
}

SequenceLayerPanel * SequenceLayer::getPanel()
{
	return new SequenceLayerPanel(this);
}

SequenceLayerTimeline * SequenceLayer::getTimelineUI()
{
	return new SequenceLayerTimeline(this);
}

//InspectableEditor * SequenceLayer::getEditor(bool isRoot)
//{
//	return new BaseItemEditor(this, isRoot);
//}

