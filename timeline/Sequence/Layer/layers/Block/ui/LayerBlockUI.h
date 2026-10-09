/*
  ==============================================================================

    LayerBlockUI.h
    Created: 14 Feb 2019 11:14:58am
    Author:  bkupe

  ==============================================================================
*/

#pragma once


class LayerBlockUI :
	public BaseItemMinimalUI<LayerBlock>,
	public UITimerTarget
{
public:
	LayerBlockUI(LayerBlock * block);
	~LayerBlockUI();

	//interaction
	float coreLengthAtMouseDown;
	float loopLengthAtMouseDown;

	bool isDragging;
	int posAtMouseDown;

	float viewStart;
	float viewEnd;
	float viewCoreEnd;

	bool canBeGrabbed;

	Colour baseColor;
	Colour highlightColor;

	Grabber grabber;
	Grabber coreGrabber;
	Grabber loopGrabber;
	class FadeHandle : public Component
	{
	public:
		FadeHandle() { setMouseCursor(MouseCursor::LeftRightResizeCursor); }
		void paint(Graphics& g) override
		{
			g.setColour(Colours::black.withAlpha(.8f)); g.fillEllipse(getLocalBounds().toFloat());
			g.setColour(YELLOW_COLOR); g.drawEllipse(getLocalBounds().toFloat().reduced(1), 2);
		}
	};
	FadeHandle fadeInHandle, fadeOutHandle;
	std::unique_ptr<AutomationUI> automationUI;
	std::unique_ptr<GradientColorManagerUI> gradientUI;
	void setInlineEditor(Automation* automation, GradientColorManager* gradient = nullptr);
	void updateInlineEditorBounds();
	void updateFadeHandles();
	double xForLocalTime(double time) const;
	double localTimeForX(double x) const;
	double fadeBeforeDrag = 0;
	double fadeDurationAtMouseDown = 0, fadeSecondsPerPixelAtMouseDown = 0;
	bool fadeEnabledBeforeDrag = false;

	virtual void paint(Graphics &g) override;
	virtual void paintOverChildren(Graphics& g) override;
	

	virtual void handlePaintTimerInternal() override;

	virtual void resized() override;
	virtual void resizedBlockInternal() {};

	virtual void mouseEnter(const MouseEvent &e) override;
	virtual void mouseExit(const MouseEvent &e) override;
	virtual void mouseDown(const MouseEvent &e) override;
	virtual void mouseDrag(const MouseEvent &e) override;
	virtual void mouseUp(const MouseEvent &e) override;

	virtual Rectangle<int> getDragBounds();
	virtual Rectangle<int> getGrabberBounds();

	virtual void controllableFeedbackUpdateInternal(Controllable *) override;
    void controllableStateUpdateInternal(Controllable*) override;

	Rectangle<int> getCoreBounds();
	int getCoreWidth();

	Rectangle<int> getLoopBounds();

	void setViewRange(float relativeStart, float relativeEnd);
	virtual void setViewRangeInternal() {}



	class BlockUIListener
	{
	public:
		virtual ~BlockUIListener() {}
		virtual void blockUITimeChanged(LayerBlockUI *) {}
		virtual void blockUINeedsReorder() {}
		
		virtual void blockUIMouseDown(LayerBlockUI*, const MouseEvent&) {}
		virtual void blockUIDragged(LayerBlockUI *, const MouseEvent &) {}
		virtual void blockUIStartDragged(LayerBlockUI *, const MouseEvent &) {}
		virtual void blockUICoreDragged(LayerBlockUI *, const MouseEvent &) {}
		virtual void blockUILoopDragged(LayerBlockUI *, const MouseEvent &) {}
	};

	ListenerList<BlockUIListener> blockUIListeners;
	void addBlockUIListener(BlockUIListener* newListener) { blockUIListeners.add(newListener); }
	void removeBlockUIListener(BlockUIListener* listener) { blockUIListeners.remove(listener); }
};
