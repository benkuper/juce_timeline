/*
  ==============================================================================

	TimeCueUI.cpp
	Created: 6 Apr 2017 11:13:51am
	Author:  Ben

  ==============================================================================
*/


TimeCueUI::TimeCueUI(TimeCue * timeCue) :
	BaseItemMinimalUI(timeCue),
	timeAtMouseDown(timeCue->time->floatValue()),
	itemLabel("Label",timeCue->niceName)
{
	dragAndDropEnabled = false;

	bgColor = bgColor.brighter();
	setRepaintsOnMouseActivity(true);
	autoDrawContourWhenSelected = false;

	itemLabel.setColour(itemLabel.backgroundColourId, Colours::transparentWhite);
	itemLabel.setColour(itemLabel.textColourId, TEXT_COLOR);

	itemLabel.setColour(itemLabel.backgroundWhenEditingColourId, Colours::black);
	itemLabel.setColour(itemLabel.textWhenEditingColourId, Colours::white);
	itemLabel.setColour(CaretComponent::caretColourId, Colours::orange);
	itemLabel.setFont(FontOptions(14));
	itemLabel.setJustificationType(Justification::centredLeft);

	itemLabel.setEditable(false, item->nameCanBeChangedByUser);
	itemLabel.addListener(this);
	addAndMakeVisible(&itemLabel);

	if(GlobalSettings::getInstance()->enableTooltips->boolValue()) setTooltip(item->niceName);

	//setDisableDefaultMouseEvents(true);
	//removeMouseListener(this);

	setSize(arrowSize + 12 + TextLayout::getStringWidth(itemLabel.getFont(),itemLabel.getText()), getHeight());

	//setSize(10, 20);

}

TimeCueUI::~TimeCueUI()
{
}

void TimeCueUI::paint(Graphics& g)
{

	Colour c = bgColor;
	if (item->isSelected) c = HIGHLIGHT_COLOR;
	else
	{
		TimeCue::CueAction a = item->cueAction->getValueDataAsEnum<TimeCue::CueAction>();
		if (a == TimeCue::PAUSE) c = BLUE_COLOR;
		else if (a == TimeCue::LOOP_JUMP) c = YELLOW_COLOR;
	}

	if (isMouseOver()) c = c.brighter();

	if (item->isUILocked->boolValue()) c = c.interpolatedWith(RED_COLOR, .5f);

	g.setColour(c);
	g.fillPath(drawPath);

	g.setColour(c.darker());
	g.strokePath(drawPath, PathStrokeType(1));

}

void TimeCueUI::resized()
{
	Rectangle<int> r = getLocalBounds();
	Rectangle<int> cr = r.removeFromLeft(arrowSize);
	drawPath.clear();
	drawPath.startNewSubPath(cr.getWidth() / 2.f, (float)cr.getHeight());
	drawPath.lineTo(0, (float)cr.getHeight() - 5);
	drawPath.lineTo(0, 0);
	drawPath.lineTo((float)cr.getWidth(), 0);
	drawPath.lineTo((float)cr.getWidth(), (float)cr.getHeight() - 5);
	drawPath.closeSubPath();
	drawPath = drawPath.createPathWithRoundedCorners(1);


	r.removeFromLeft(1);
	itemLabel.setBounds(r);

}

void TimeCueUI::mouseDoubleClick(const MouseEvent& e)
{
	BaseItemMinimalUI::mouseDoubleClick(e);
	if (e.mods.isCommandDown()) item->remove();
}

void TimeCueUI::mouseDown(const MouseEvent& e)
{
	if (e.eventComponent == &itemLabel || e.eventComponent == itemLabel.getCurrentTextEditor()) return;
	BaseItemMinimalUI::mouseDown(e);
	timeAtMouseDown = item->time->floatValue();
	if (!item->isUILocked->boolValue() && e.eventComponent != &itemLabel) cueUIListeners.call(&TimeCueUIListener::cueMouseDown, this, e);
}

void TimeCueUI::mouseDrag(const MouseEvent& e)
{
	if (e.eventComponent == &itemLabel || e.eventComponent == itemLabel.getCurrentTextEditor()) return;
	BaseItemMinimalUI::mouseDrag(e);
	if (!item->isUILocked->boolValue() && e.eventComponent != &itemLabel) cueUIListeners.call(&TimeCueUIListener::cueDragged, this, e);
}

void TimeCueUI::mouseUp(const MouseEvent& e)
{
	if (e.eventComponent == &itemLabel || e.eventComponent == itemLabel.getCurrentTextEditor()) return;
	BaseItemMinimalUI::mouseUp(e);

	if (!item->isUILocked->boolValue() && item->time->floatValue() != timeAtMouseDown) item->time->setUndoableValue(item->time->floatValue());

	if (!item->isUILocked->boolValue() && e.eventComponent != &itemLabel) cueUIListeners.call(&TimeCueUIListener::cueMouseUp, this, e);
}

void TimeCueUI::labelTextChanged(Label* l)
{
	if (l == &itemLabel)
	{
		if (l->getText().isEmpty()) itemLabel.setText(this->baseItem->niceName, dontSendNotification); //avoid setting empty names
		else this->baseItem->setUndoableNiceName(l->getText());
		setSize(arrowSize + 12 + TextLayout::getStringWidth(itemLabel.getFont(),itemLabel.getText()), getHeight());
	}
}

void TimeCueUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	if (c == item->time)
	{
		cueUIListeners.call(&TimeCueUIListener::cueTimeChanged, this);
	}
	else if (c == item->isUILocked || c == item->cueAction)
	{
		repaint();
	}
	else if (c == item->loopCue)
	{
		if (getParentComponent() != nullptr) getParentComponent()->repaint(); //force repaint manager
	}
}

void TimeCueUI::containerChildAddressChangedAsync(ControllableContainer* cc)
{
	itemLabel.setText(item->niceName, dontSendNotification);
	setSize(arrowSize + 12 + TextLayout::getStringWidth(itemLabel.getFont(),itemLabel.getText()), getHeight());
}
