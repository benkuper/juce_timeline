/*
  ==============================================================================

	LayerBlockUI.cpp
	Created: 14 Feb 2019 11:14:58am
	Author:  bkupe

  ==============================================================================
*/

#include "JuceHeader.h"

LayerBlockUI::LayerBlockUI(LayerBlock * block) :
	BaseItemMinimalUI(block),
	UITimerTarget(ORGANICUI_SLOW_TIMER, "LayerBlockUI"),
	viewStart(0),
	viewEnd(block->getTotalLength()),
	viewCoreEnd(block->coreLength->floatValue()),
	canBeGrabbed(true),
	baseColor(block->itemColor != nullptr ? block->itemColor->getColor() : BG_COLOR.brighter(.1f)),
	highlightColor(baseColor),
	grabber(Grabber::VERTICAL),
	coreGrabber(Grabber::VERTICAL),
	loopGrabber(Grabber::VERTICAL)
{
	dragAndDropEnabled = false;
	isDragging = false;
	addChildComponent(fadeInHandle);
	addChildComponent(fadeOutHandle);

	bgColor = item->isActive->boolValue() ? highlightColor.brighter(.3f) : baseColor;

	if (canBeGrabbed)
	{
		addChildComponent(&grabber);
		addChildComponent(&coreGrabber);
		addChildComponent(&loopGrabber);
	}
}

LayerBlockUI::~LayerBlockUI()
{
}


void LayerBlockUI::paint(Graphics & g)
{
	if (inspectable.wasObjectDeleted()) return;

	BaseItemMinimalUI::paint(g);
	g.fillCheckerBoard(getMainBounds().withLeft(getCoreWidth()).toFloat(), 16, 16, Colours::white.withAlpha(.05f), Colours::white.withAlpha(.1f));
}

void LayerBlockUI::paintOverChildren(Graphics& g)
{
	if (inspectable.wasObjectDeleted()) return;
	BaseItemMinimalUI::paintOverChildren(g);

	if (item->blockFadeIn || item->blockFadeOut)
	{
		const auto fades = item->getEffectiveFades();
		const double length = item->coreLength->doubleValue();
		auto ramp = [&](double start, double end, bool incoming)
		{
			if (end <= start || end <= viewStart || start >= viewCoreEnd) return;
			Path shade, line;
			const double first = jmax(start, (double)viewStart), last = jmin(end, (double)viewCoreEnd);
			shade.startNewSubPath((float)xForLocalTime(first), 0);
			for (int i = 0; i <= 40; ++i)
			{
				const double t = first + (last - first) * i / 40.0;
				const double w = item->getFadeCurveValue((t - start) / (end - start));
				const float x = (float)xForLocalTime(t), y = (float)((1 - (incoming ? w : 1 - w)) * getHeight());
				shade.lineTo(x, y);
				if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
			}
			shade.lineTo((float)xForLocalTime(last), 0); shade.closeSubPath();
			g.setColour(YELLOW_COLOR.withAlpha(.15f)); g.fillPath(shade);
			g.setColour(YELLOW_COLOR.withAlpha(.8f)); g.strokePath(line, PathStrokeType(1.5f));
		};
		ramp(0, fades.in, true); ramp(length - fades.out, length, false);
	}
	if (item->isActive->boolValue())
	{
		g.setColour(Colours::cyan.withAlpha(.85f)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 3, 2);
		g.fillEllipse(4, 4, 6, 6);
	}

	if (item->isUILocked->boolValue())
	{
		g.setTiledImageFill(ImageCache::getFromMemory(TimelineBinaryData::stripe_png, TimelineBinaryData::stripe_pngSize), 0, 0, .1f);
		g.fillAll();
	}

	validatePaint();
}

void LayerBlockUI::handlePaintTimerInternal()
{
	updateFadeHandles();
	repaint();
}

void LayerBlockUI::resized()
{
	if (canBeGrabbed)
	{
		Rectangle<int> r = getGrabberBounds();

		const int grabberSize = 9;

		grabber.setBounds(r.removeFromLeft(grabberSize));

		loopGrabber.setVisible(item->loopLength->floatValue() > 0);
		if (loopGrabber.isVisible())	loopGrabber.setBounds(r.removeFromRight(grabberSize));

		r.setRight(getCoreWidth());
		coreGrabber.setBounds(r.removeFromRight(grabberSize));
	}

	resizedBlockInternal();
	updateInlineEditorBounds();
	updateFadeHandles();
}

void LayerBlockUI::mouseEnter(const MouseEvent & e)
{
	BaseItemMinimalUI::mouseEnter(e);

	if (inspectable.wasObjectDeleted()) return;


	if (canBeGrabbed && !item->isUILocked->boolValue() && getWidth() > 24)
	{
		grabber.setVisible(true);
		coreGrabber.setVisible(true);
		loopGrabber.setVisible(true && item->loopLength->floatValue() > 0);
	}
	updateFadeHandles();
}

void LayerBlockUI::mouseExit(const MouseEvent & e)
{
	BaseItemMinimalUI::mouseExit(e);
	
	if (canBeGrabbed)
	{
		grabber.setVisible(isMouseOverOrDragging());
		coreGrabber.setVisible(isMouseOverOrDragging());
		loopGrabber.setVisible(isMouseOverOrDragging() && item->loopLength->floatValue() > 0);

		if (isMouseOverOrDragging())
		{
			grabber.toFront(false);
			coreGrabber.toFront(false);
			loopGrabber.toFront(false);
		}
	}
}


void LayerBlockUI::mouseDown(const MouseEvent & e)
{
	if (e.eventComponent == &fadeInHandle || e.eventComponent == &fadeOutHandle)
	{
		if (item->isUILocked->boolValue()) return;
		InspectableContentComponent::mouseDown(e);
		auto* p = e.eventComponent == &fadeInHandle ? item->blockFadeIn : item->blockFadeOut;
		fadeBeforeDrag = p->doubleValue(); fadeEnabledBeforeDrag = p->enabled;
        const auto fades = item->getEffectiveFades();
        fadeDurationAtMouseDown = e.eventComponent == &fadeInHandle ? fades.in : fades.out;
        fadeSecondsPerPixelAtMouseDown = getWidth() > 0 ? (viewEnd - viewStart) / getWidth() : 0;
		isDragging = false; return;
	}
    if (e.eventComponent == this && e.mods.isRightButtonDown())
    {
        PopupMenu menu; addContextMenuItems(menu);
        Component::SafePointer<LayerBlockUI> safe(this);
        if (menu.getNumItems() > 0)
            menu.showMenuAsync(PopupMenu::Options(), [safe](int result) { if (safe && result > 0) safe->handleContextMenuResult(result); });
        return;
    }
	BaseItemMinimalUI::mouseDown(e);

	if (canBeGrabbed && !item->isUILocked->boolValue())
	{
		item->setMovePositionReference(true);
		coreLengthAtMouseDown = item->coreLength->floatValue();
		loopLengthAtMouseDown = item->loopLength->floatValue();

		isDragging = e.mods.isLeftButtonDown() && e.eventComponent == this && getDragBounds().contains(e.getPosition()) && !e.mods.isCommandDown() && !e.mods.isShiftDown();
		posAtMouseDown = getX();

		blockUIListeners.call(&BlockUIListener::blockUIMouseDown, this, e);
	}
}

void LayerBlockUI::mouseDrag(const MouseEvent & e)
{
	if (e.eventComponent == &fadeInHandle || e.eventComponent == &fadeOutHandle)
	{
		if (item->isUILocked->boolValue()) return;
		const bool incoming = e.eventComponent == &fadeInHandle;
		auto* p = incoming ? item->blockFadeIn : item->blockFadeOut;
        const double duration = fadeDurationAtMouseDown + (incoming ? 1 : -1)
            * e.getOffsetFromDragStart().x * fadeSecondsPerPixelAtMouseDown;
		p->setEnabled(true);
		p->setValue(jlimit(0.0, item->coreLength->doubleValue(), duration));
		updateFadeHandles(); repaint(); return;
	}
	if (canBeGrabbed && !item->isUILocked->boolValue())
	{
		if (isDragging)
		{
			blockUIListeners.call(&BlockUIListener::blockUIDragged, this, e);
		}
		else if (e.eventComponent == &grabber)
		{
			blockUIListeners.call(&BlockUIListener::blockUIStartDragged, this, e);
		}
		else if (e.eventComponent == &coreGrabber)
		{
			blockUIListeners.call(&BlockUIListener::blockUICoreDragged, this, e);
		}
		else if (e.eventComponent == &loopGrabber)
		{
			blockUIListeners.call(&BlockUIListener::blockUILoopDragged, this, e);
		}
	}
	BaseItemMinimalUI::mouseDrag(e);

}

void LayerBlockUI::mouseUp(const MouseEvent & e)
{
	if (e.eventComponent == &fadeInHandle || e.eventComponent == &fadeOutHandle)
	{
		if (item->isUILocked->boolValue()) return;
		auto* p = e.eventComponent == &fadeInHandle ? item->blockFadeIn : item->blockFadeOut;
		class FadeAction : public Controllable::ControllableAction
		{
		public:
			FadeAction(FloatParameter* p, double oldValue, bool oldEnabled) : ControllableAction(p), before(oldValue), after(p->doubleValue()), wasEnabled(oldEnabled), nowEnabled(p->enabled) {}
			double before, after; bool wasEnabled, nowEnabled;
			bool apply(double v, bool e) { if (auto* p = dynamic_cast<FloatParameter*>(getControllable())) { p->setEnabled(e); p->setValue(v); return true; } return false; }
			bool perform() override { return apply(after, nowEnabled); }
			bool undo() override { return apply(before, wasEnabled); }
		};
		if (fadeBeforeDrag != p->doubleValue() || fadeEnabledBeforeDrag != p->enabled)
			UndoMaster::getInstance()->performAction("Edit block fade", new FadeAction(p, fadeBeforeDrag, fadeEnabledBeforeDrag));
		return;
	}

	if (canBeGrabbed && !item->isUILocked->boolValue())
	{
		if (isDragging)
		{
			item->addMoveToUndoManager(true);
			blockUIListeners.call(&BlockUIListener::blockUINeedsReorder);
		}
		else if (e.eventComponent == &grabber)
		{
			item->time->setUndoableValue(item->movePositionReference.x, item->time->floatValue());
			item->coreLength->setUndoableValue(coreLengthAtMouseDown, item->coreLength->floatValue());
		}
		else if (e.eventComponent == &coreGrabber)
		{
			item->time->setUndoableValue(item->movePositionReference.x, item->time->floatValue());
			item->coreLength->setUndoableValue(coreLengthAtMouseDown, item->coreLength->floatValue());
		}
		else if (e.eventComponent == &loopGrabber)
		{
			item->loopLength->setUndoableValue(loopLengthAtMouseDown, item->loopLength->floatValue());
		}

		isDragging = false;

		grabber.setVisible(isMouseOverOrDragging());
		coreGrabber.setVisible(isMouseOverOrDragging());
		loopGrabber.setVisible(isMouseOverOrDragging());
	}

	BaseItemMinimalUI::mouseUp(e);
}

Rectangle<int> LayerBlockUI::getDragBounds()
{
	return (automationUI || gradientUI) ? getLocalBounds().withHeight(22) : getLocalBounds();
}

Rectangle<int> LayerBlockUI::getGrabberBounds()
{
	return getLocalBounds().reduced(0, 8);
}


void LayerBlockUI::controllableStateUpdateInternal(Controllable* c)
{
    if (c == item->blockFadeIn || c == item->blockFadeOut) { updateFadeHandles(); shouldRepaint = true; }
}

void LayerBlockUI::controllableFeedbackUpdateInternal(Controllable * c)
{
    if (c == item->blockFadeIn || c == item->blockFadeOut) { updateFadeHandles(); shouldRepaint = true; }
	if (c == item->time || c == item->coreLength || c == item->loopLength)
	{
		blockUIListeners.call(&BlockUIListener::blockUITimeChanged, this);
	}
	else if (c == item->isActive || c == item->itemColor)
	{
		if (c == item->itemColor)
		{
			baseColor = item->itemColor->getColor();
			highlightColor = baseColor;
		}
		bgColor = item->isActive->boolValue() ? highlightColor.brighter(.3f) : baseColor;
		shouldRepaint = true;
	}
	else if (c == item->isUILocked)
	{
		repaint();
	}
}

Rectangle<int> LayerBlockUI::getCoreBounds()
{
	return getLocalBounds().withWidth(getCoreWidth());
}

int LayerBlockUI::getCoreWidth()
{
	return roundToInt(jlimit(0.0, (double)getWidth(), xForLocalTime(item->coreLength->doubleValue())));
}

Rectangle<int> LayerBlockUI::getLoopBounds()
{
	return getLocalBounds().withLeft(getCoreWidth());

}

void LayerBlockUI::setViewRange(float relativeStart, float relativeEnd)
{
	relativeStart = jmax<float>(relativeStart, 0);
	relativeEnd = jmin<float>(relativeEnd, item->getTotalLength());

	if (viewStart == relativeStart && viewEnd == relativeEnd) return;


	viewStart = relativeStart;
	viewEnd = relativeEnd;

	viewCoreEnd = jmin(viewEnd, item->coreLength->floatValue());
	setViewRangeInternal();
	resized();

	shouldRepaint = true;
}

double LayerBlockUI::xForLocalTime(double t) const
{ return viewEnd > viewStart ? (t - viewStart) * getWidth() / (viewEnd - viewStart) : 0; }

double LayerBlockUI::localTimeForX(double x) const
{ return getWidth() > 0 ? viewStart + x * (viewEnd - viewStart) / getWidth() : viewStart; }

void LayerBlockUI::setInlineEditor(Automation* automation, GradientColorManager* gradient)
{
	automationUI.reset(); gradientUI.reset();
	if (automation)
	{
		automationUI.reset(new AutomationUI(automation));
		automationUI->disableOverlayFill = true;
		automationUI->keysUI.autoAdaptViewRange = false;
		addAndMakeVisible(automationUI.get());
	}
	if (gradient)
	{
		gradientUI.reset(new GradientColorManagerUI(gradient));
		// The preview worker exits when its initial width is zero. Start it
		// after the embedded editor has received its actual visible bounds.
		gradientUI->stopThread(1000);
		gradientUI->autoResetViewRangeOnLengthUpdate = false;
		addAndMakeVisible(gradientUI.get());
	}
	resized(); repaint();
}

void LayerBlockUI::updateInlineEditorBounds()
{
	auto bounds = getCoreBounds(); bounds.removeFromTop(jmin(22, bounds.getHeight()));
	if (automationUI)
	{
		automationUI->setBounds(bounds);
		// AutomationUI normally defers these bounds to its paint timer. Embedded
		// editors must match the block on the very first frame of a zoom/trim.
		automationUI->keysUI.setBounds(automationUI->getLocalBounds());
		automationUI->overlay.setBounds(automationUI->getLocalBounds());
		automationUI->background.setBounds(automationUI->getLocalBounds());
		automationUI->shouldResize = false;
		automationUI->setViewRange(viewStart, jmax(viewStart + .0001f, viewCoreEnd));
	}
	if (gradientUI)
	{
		gradientUI->setBounds(bounds);
		gradientUI->setViewRange(viewStart, jmax(viewStart + .0001f, viewCoreEnd));
		if (bounds.getWidth() > 0 && !gradientUI->isThreadRunning())
		{
			gradientUI->shouldUpdateImage = true;
			gradientUI->startThread();
		}
	}
}

void LayerBlockUI::updateFadeHandles()
{
	if (inspectable.wasObjectDeleted()) return;
    const bool handles = canBeGrabbed && !item->isUILocked->boolValue() && isMouseOverOrDragging();
    grabber.setVisible(handles && viewStart <= 0);
    coreGrabber.setVisible(handles && viewEnd >= item->coreLength->floatValue() && viewStart < item->coreLength->floatValue());
    loopGrabber.setVisible(handles && viewEnd >= item->getTotalLength() && item->loopLength->floatValue() > 0);
	const auto fades = item->getEffectiveFades();
	auto place = [&](FadeHandle& handle, FloatParameter* p, double t)
	{
		handle.setVisible(p && !item->isUILocked->boolValue() && t >= viewStart && t <= viewEnd && getWidth() > 24);
		handle.setBounds(jlimit(0, jmax(0, getWidth() - 10), roundToInt(xForLocalTime(t)) - 5), 1, 10, 10); handle.toFront(false);
	};
	place(fadeInHandle, item->blockFadeIn, fades.in);
	place(fadeOutHandle, item->blockFadeOut, item->coreLength->doubleValue() - fades.out);
}
