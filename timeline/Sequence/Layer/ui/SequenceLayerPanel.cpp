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
	miniModeUI.reset(item->miniMode->createToggle(AssetManager::getInstance()->getMinusImage()));

	lockUI.reset(item->isUILocked->createToggle(ImageCache::getFromMemory(OrganicUIBinaryData::padlock_png, OrganicUIBinaryData::padlock_pngSize)));
	addAndMakeVisible(lockUI.get());

	addAndMakeVisible(colorUI.get());
	addAndMakeVisible(miniModeUI.get());

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
	BaseItemUI::resizedInternalHeader(r);
	miniModeUI->setBounds(r.removeFromRight(r.getHeight()));
	r.removeFromRight(2);
	lockUI->setBounds(r.removeFromRight(r.getHeight()).reduced(2));
	r.removeFromRight(2);
	colorUI->setBounds(r.removeFromRight(r.getHeight()).reduced(2));
}

void SequenceLayerPanel::controllableFeedbackUpdateInternal(Controllable * c)
{
	BaseItemUI::controllableFeedbackUpdateInternal(c);

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
