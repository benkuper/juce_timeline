/*
  ==============================================================================

	AudioLayerClipUI.cpp
	Created: 8 Feb 2017 12:20:09pm
	Author:  Ben

  ==============================================================================
*/

#include "JuceHeader.h"

AudioLayerClipUI::AudioLayerClipUI(AudioLayerClip* _clip) :
	LayerBlockUI(_clip),
	thumbnailCache(100000),
	thumbnail(50, _clip->formatManager, thumbnailCache),
	clip(_clip)
{
	dragAndDropEnabled = false;
	bgColor = clip->isActive->boolValue() ? AUDIO_COLOR.brighter() : BG_COLOR.brighter(.1f);

	clip->addAsyncClipListener(this);

	thumbnail.addChangeListener(this);

#if JUCE_WINDOWS
	if (clip->filePath->stringValue().startsWithChar('/')) return;
#endif

}

AudioLayerClipUI::~AudioLayerClipUI()
{
	if (!inspectable.wasObjectDeleted()) clip->removeAsyncClipListener(this);
}

void AudioLayerClipUI::paint(Graphics& g)
{
	LayerBlockUI::paint(g);

	if (clip->filePath->stringValue().isEmpty()) return;
	g.setColour(Colours::white.withAlpha(.5f));
	if (clip->isLoading)
	{
		g.setFont(FontOptions(20));
		g.drawText("Loading...", getLocalBounds(), Justification::centred);

	}
	else
	{
		float volume = clip->volume->controlMode == Parameter::ControlMode::MANUAL ? clip->volume->floatValue() : 1;
		float stretch = clip->stretchFactor->floatValue();
		float startOffset = clip->clipStartOffset->floatValue();

		thumbnail.drawChannels(g, getCoreBounds(), startOffset + viewStart, startOffset + viewStart + viewCoreLength / stretch, volume);
	}

	if (clip->fadeIn->floatValue() > 0)
	{
		g.setColour(YELLOW_COLOR.withAlpha(.2f));
		int fadeInWidth = clip->fadeIn->floatValue() * getCoreWidth() / clip->coreLength->floatValue();
		Path p;
		p.startNewSubPath(0, 0);
		p.lineTo(fadeInWidth, 0);
		p.lineTo(0, getHeight());
		p.closeSubPath();
		g.fillPath(p);
	}

	if (clip->fadeOut->floatValue() > 0)
	{
		g.setColour(YELLOW_COLOR.withAlpha(.2f));
		int fadeOutWidth = clip->fadeOut->floatValue() * getCoreWidth() / clip->coreLength->floatValue();
		Path p;
		p.startNewSubPath(getCoreWidth(), 0);
		p.lineTo(getCoreWidth() - fadeOutWidth, 0);
		p.lineTo(getCoreWidth(), getHeight());
		p.closeSubPath();
		g.fillPath(p);
	}

	if (automationUI != nullptr)
	{
		Rectangle<int> r = getCoreBounds();
		if (automationUI != nullptr)
		{
			/*if (dynamic_cast<GradientColorManagerUI*>(automationUI.get()) != nullptr) automationUI->setBounds(r.removeFromBottom(20));
			else
			*/
			automationUI->setBounds(r);
		}
	}
}

void AudioLayerClipUI::resizedBlockInternal()
{
}

void AudioLayerClipUI::mouseDown(const MouseEvent& e)
{
	LayerBlockUI::mouseDown(e);
	if (e.mods.isRightButtonDown() && (e.eventComponent == this || e.eventComponent == automationUI.get()))
	{
		PopupMenu::dismissAllActiveMenus();
		PopupMenu p;
		p.addItem(1, "Clear automation editor", automationUI != nullptr);
		p.addItem(2, "Edit enveloppe automation", automationUI == nullptr);
		p.addItem(3, "Remove enveloppe automation", clip->volume->controlMode != Parameter::ControlMode::MANUAL);
		if(automationUI != nullptr)
		{
			p.addSeparator();
			automationUI->keysUI.addMenuExtraItems(p, 4);
		}

		p.showMenuAsync(PopupMenu::Options(), [this](int result)
			{
				AudioLayerClip* clip = this->clip;

				switch (result)
				{
				case 1:
					this->setTargetAutomation(nullptr);
					break;

				case 2:
				{
					if (clip->volume->controlMode != Parameter::ControlMode::AUTOMATION)
					{
						clip->volume->setControlMode(Parameter::ControlMode::AUTOMATION);
						clip->volume->automation->setManualMode(true);

						Automation* a = dynamic_cast<Automation*>(clip->volume->automation->automationContainer);

						if (a != nullptr)
						{
							a->clear();
							a->setLength(clip->coreLength->floatValue());
							AutomationKey* k = a->addItem(0, 0);
							k->setEasing(Easing::BEZIER);
							a->addKey(a->length->floatValue(), 1);
						}
					}

					this->setTargetAutomation(clip->volume->automation.get());
				}
				break;

				case 3:
					this->setTargetAutomation(nullptr);
					clip->volume->setControlMode(Parameter::ControlMode::MANUAL);
					break;

				default:
					if(result>3)
						automationUI->keysUI.handleMenuExtraItemsResult(result, 4);
					break;
				}
			}
		);
	}
}

void AudioLayerClipUI::setupThumbnail()
{
	thumbnail.setSource(new FileInputSource(clip->filePath->getFile()));
	shouldRepaint = true;
}

void AudioLayerClipUI::setTargetAutomation(ParameterAutomation* a)
{
	if (automationUI != nullptr)
	{
		removeChildComponent(automationUI.get());
		automationUI = nullptr;
	}

	canBeGrabbed = true;

	if (a == nullptr) return;


	if (dynamic_cast<ParameterNumberAutomation*>(a) != nullptr)
	{
		AutomationUI* aui = new AutomationUI((Automation*)a->automationContainer);
		//aui->updateROI();
		aui->showMenuOnRightClick = false;
		automationUI.reset(aui);
	}
	/*
	else if (dynamic_cast<ParameterColorAutomation*>(a) != nullptr)
	{
		GradientColorManagerUI* gui = new GradientColorManagerUI((GradientColorManager*)a->automationContainer);
		gui->autoResetViewRangeOnLengthUpdate = true;
		automationUI.reset(gui);
	}
	*/

	if (automationUI != nullptr)
	{
		canBeGrabbed = false;
		coreGrabber.setVisible(false);
		grabber.setVisible(false);
		loopGrabber.setVisible(false);
		automationUI->addMouseListener(this, false);
		addAndMakeVisible(automationUI.get());
		resized();
	}
}


void AudioLayerClipUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	LayerBlockUI::controllableFeedbackUpdateInternal(c);

	if (c == item->time || c == item->coreLength || c == clip->fadeIn || c == clip->fadeOut)
	{
		shouldRepaint = true;
	}
	else if (c == clip->volume && clip->volume->controlMode == Parameter::ControlMode::MANUAL)
	{
		shouldRepaint = true;
	}

	if (c == clip->isActive)
	{
		bgColor = clip->isActive->boolValue() ? AUDIO_COLOR.brighter() : BG_COLOR.brighter(.1f);
		shouldRepaint = true;
	}
}

void AudioLayerClipUI::newMessage(const AudioLayerClip::ClipEvent& e)
{
	switch (e.type)
	{
	case AudioLayerClip::ClipEvent::SOURCE_LOAD_START:
		thumbnail.setSource(nullptr);
		shouldRepaint = true;
		break;

	case AudioLayerClip::ClipEvent::SOURCE_LOAD_END:
		setupThumbnail();
		break;

	}
}

void AudioLayerClipUI::changeListenerCallback(ChangeBroadcaster* source)
{
	shouldRepaint = true;
}
