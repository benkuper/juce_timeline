/*
  ==============================================================================

	SequenceLayerPanel.cpp
	Created: 17 Nov 2016 7:59:08pm
	Author:  Ben Kuper

  ==============================================================================
*/

#include "JuceHeader.h"

SequenceLayerPanel::SequenceLayerPanel(SequenceLayer* layer, Direction direction) :
	ItemUI<SequenceLayer>(layer, direction)
{
	setWantsKeyboardFocus(false);
	setMouseClickGrabsKeyboardFocus(false);
	bringToFrontOnSelect = false;

	bgColor = item->itemColor->getColor();
	miniModeUI.reset(item->miniMode->createToggle(AssetManager::getInstance()->minusImage));

	lockUI.reset(item->isUILocked->createToggle(ImageCache::getFromMemory(OrganicUIBinaryData::padlock_png, OrganicUIBinaryData::padlock_pngSize)));
	addAndMakeVisible(lockUI.get());

	addAndMakeVisible(miniModeUI.get());

	setSize(100, item->uiHeight->intValue());
}


SequenceLayerPanel::~SequenceLayerPanel()
{

}

void SequenceLayerPanel::paintOverChildren(Graphics& g)
{
	g.setColour(item->itemColor->getColor());
	g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, 2);
	ItemUI::paintOverChildren(g);
}

void SequenceLayerPanel::resized()
{
	ItemUI::resized();
	item->uiHeight->setValue(getHeight());
}

void SequenceLayerPanel::resizedInternalHeader(Rectangle<int>& r)
{
	ItemUI::resizedInternalHeader(r);
	miniModeUI->setBounds(r.removeFromRight(r.getHeight()));
	r.removeFromRight(2);
	lockUI->setBounds(r.removeFromRight(r.getHeight()).reduced(2));
}

void SequenceLayerPanel::controllableFeedbackUpdateInternal(Controllable* c)
{
	ItemUI::controllableFeedbackUpdateInternal(c);

	if (c == item->uiHeight)
	{
		if (!item->isCurrentlyLoadingData) setSize(getWidth(), item->uiHeight->intValue());
	}
}

SequenceLayerGroupPanel::SequenceLayerGroupPanel(ItemBaseGroup<SequenceLayer>* group) :
	ItemGroupUI<SequenceLayer>(group, NONE)
{
	setShowGroupManager(true);
	setSize(100, 100);
}

SequenceLayerGroupPanel::~SequenceLayerGroupPanel()
{
}

BaseManagerUI* SequenceLayerGroupPanel::createGroupManagerUIInternal()
{
	return new SequenceLayerPanelManagerUI((SequenceLayerManager*)this->group->manager, false);
}
