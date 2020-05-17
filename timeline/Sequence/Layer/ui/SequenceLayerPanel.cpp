/*
  ==============================================================================

    SequenceLayerPanel.cpp
    Created: 17 Nov 2016 7:59:08pm
    Author:  Ben Kuper

  ==============================================================================
*/

SequenceLayerPanel::SequenceLayerPanel(SequenceLayer * layer) :
	BaseItemUI<SequenceLayer>(layer,Direction::VERTICAL)
{
	setWantsKeyboardFocus(false); 
	setMouseClickGrabsKeyboardFocus(false);
	bringToFrontOnSelect = false;

	bgColor = item->color->getColor(); 
	colorUI.reset(item->color->createColorParamUI());

	lockUI.reset(item->isUILocked->createImageToggle(AssetManager::getInstance()->getToggleBTImage(ImageCache::getFromMemory(OrganicUIBinaryData::padlock_png, OrganicUIBinaryData::padlock_pngSize))));
	addAndMakeVisible(lockUI.get());

	addAndMakeVisible(colorUI.get());
	setSize(100, item->uiHeight->intValue());
}

SequenceLayerPanel::~SequenceLayerPanel()
{

}

void SequenceLayerPanel::paintOverChildren(Graphics & g)
{
	g.setColour(item->color->getColor());
	g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, 2);
	BaseItemUI::paintOverChildren(g);
}

void SequenceLayerPanel::resized()
{
	BaseItemUI::resized();
	item->uiHeight->setValue(getHeight());
}

void SequenceLayerPanel::resizedInternalHeader(Rectangle<int>& r)
{
	lockUI->setBounds(r.removeFromRight(r.getHeight()).reduced(2));
	r.removeFromRight(2);
	colorUI->setBounds(r.removeFromRight(r.getHeight()).reduced(2));
}

void SequenceLayerPanel::controllableFeedbackUpdateInternal(Controllable * c)
{
	BaseItemMinimalUI::controllableFeedbackUpdateInternal(c);

	if (c == item->uiHeight)
	{
		if (!item->isCurrentlyLoadingData) setSize(getWidth(), item->uiHeight->intValue());
	}
	else if (c == item->color)
	{
		if (!item->isCurrentlyLoadingData)
		{
			bgColor = item->color->getColor();
			repaint();
		}
	}

}
