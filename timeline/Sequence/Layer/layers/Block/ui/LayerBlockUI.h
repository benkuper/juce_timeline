/*
  ==============================================================================

    LayerBlockUI.h
    Created: 14 Feb 2019 11:14:58am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class LayerBlockManagerUI;

class LayerBlockUI :
	public ItemMinimalUI<LayerBlock>,
	public UITimerTarget
{
public:
	LayerBlockUI(LayerBlock * block);
	~LayerBlockUI();

	LayerBlockManagerUI* blockManagerUI;

	//interaction
	float coreLengthAtMouseDown;
	float loopLengthAtMouseDown;

	bool isDragging;
	int posAtMouseDown;

	float viewStart;
	float viewEnd;
	float viewCoreLength;

	bool canBeGrabbed;

	Colour baseColor;
	Colour highlightColor;

	Grabber grabber;
	Grabber coreGrabber;
	Grabber loopGrabber;

	virtual void paint(Graphics &g) override;
	virtual void paintOverChildren(Graphics& g) override;
	

	virtual void handlePaintTimerInternal() override;

	virtual void resized() override;
	virtual void resizedBlockInternal() {};

	virtual void updateGrabbers();

	virtual void mouseEnter(const MouseEvent &e) override;
	virtual void mouseExit(const MouseEvent &e) override;
	virtual void mouseDown(const MouseEvent &e) override;
	virtual void mouseDrag(const MouseEvent &e) override;
	virtual void mouseUp(const MouseEvent &e) override;

	virtual void controllableFeedbackUpdateInternal(Controllable *) override;

	Rectangle<int> getCoreBounds();
	int getCoreWidth();

	Rectangle<int> getLoopBounds();

	int getRealXForTime(float time, bool relative = true);
	float getTimeForX(int x, bool relative = true);

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