/*
  ==============================================================================

	LayerBlock.cpp
	Created: 14 Feb 2019 11:14:35am
	Author:  bkupe

  ==============================================================================
*/

LayerBlock::LayerBlock(StringRef name) :
	BaseItem(name, true, false)
{
	editorIsCollapsed = true;
	setHasCustomColor(true);
	itemColor->setDefaultValue(BG_COLOR.brighter(.1f));

	time = addFloatParameter("Start Time", "Time of the start of the clip", 0, 0);
	time->defaultUI = FloatParameter::TIME;

	coreLength = addFloatParameter("Length", "Length of the clip's core, without looping (in seconds)", 10, .1f);
	coreLength->defaultUI = FloatParameter::TIME;
	loopLength = addFloatParameter("Loop Length", "Length of the clip's oop, after the core", 0, 0);
	loopLength->defaultUI = FloatParameter::TIME;

	isActive = addBoolParameter("Is Active", "This is a feedback to know if block is currently active in the timeline", false);
	isActive->setControllableFeedbackOnly(true);

	isUILocked->hideInEditor = false;
}

LayerBlock::~LayerBlock()
{
}

void LayerBlock::addFadeParameters(double duration)
{
	blockFadeIn = addFloatParameter("Fade In", "Enable to override the automatic incoming fade", duration, 0);
	blockFadeOut = addFloatParameter("Fade Out", "Enable to override the automatic outgoing fade", duration, 0);
	for (auto* p : { blockFadeIn, blockFadeOut })
	{
		p->defaultUI = FloatParameter::TIME;
		p->canBeDisabledByUser = true;
		p->setEnabled(false);
	}
}

BlockTransitions::Fades LayerBlock::getEffectiveFades() const
{
	BlockTransitions::Fades result;
	const double start = time->doubleValue(), end = start + coreLength->doubleValue();
	if (auto* manager = dynamic_cast<LayerBlockManager*>(parentContainer.get()))
		for (auto* other : manager->items)
		{
			if (other == this || !other->enabled->boolValue()) continue;
			const double s = other->time->doubleValue(), e = s + other->coreLength->doubleValue();
			const double crossing = BlockTransitions::overlap(start, end, s, e);
			if (s < start) result.in = jmax(result.in, crossing);
			if (s > start) result.out = jmax(result.out, crossing);
		}
	BlockTransitions::Fades manual;
	if (blockFadeIn && blockFadeIn->enabled) { manual.in = blockFadeIn->doubleValue(); result.in = 0; }
	if (blockFadeOut && blockFadeOut->enabled) { manual.out = blockFadeOut->doubleValue(); result.out = 0; }
	return BlockTransitions::withReservedOverlaps(manual, coreLength->doubleValue(), result);
}

float LayerBlock::getTotalLength()
{
	return coreLength->floatValue() + loopLength->floatValue();
}

float LayerBlock::getCoreEndTime()
{
	return time->floatValue() + coreLength->floatValue();
}

float LayerBlock::getEndTime()
{
	return time->floatValue() + getTotalLength();
}

bool LayerBlock::isInRange(float _time)
{
	return _time >= time->floatValue() && _time < getEndTime();
}

void LayerBlock::setMovePositionReferenceInternal()
{
	movePositionReference.setX(time->floatValue());
}

void LayerBlock::setPosition(Point<float> targetPosition)
{
	blockListeners.call(&BlockListener::askForPlaceBlockTime, this, targetPosition.x);
}

Point<float> LayerBlock::getPosition()
{
	return Point<float>(time->floatValue(),0);
}

void LayerBlock::addUndoableMoveAction(Array<UndoableAction *> &actions)
{
	actions.add(time->setUndoableValue(movePositionReference.x, time->floatValue(), true));
}


void LayerBlock::setCoreLength(float newLength, bool /*stretch*/, bool /*stickToCoreEnd*/)
{
	coreLength->setValue(newLength);
}

void LayerBlock::setLoopLength(float newLength)
{
	loopLength->setValue(newLength);
}

void LayerBlock::setStartTime(float newStart, bool keepCoreEnd, bool stickToCoreEnd)
{
	float timeDiff = newStart - time->floatValue();
	time->setValue(newStart);
	if (keepCoreEnd) setCoreLength(coreLength->floatValue() - timeDiff, false, stickToCoreEnd);
}

double LayerBlock::getRelativeTime(double t, bool timeIsAbsolute, bool noLoop)
{
	if (timeIsAbsolute) t -= time->floatValue();
	if (t == coreLength->floatValue() || noLoop) return t; //avoid getting the start value when asking for the end value
	return fmod(t, coreLength->floatValue());
}
