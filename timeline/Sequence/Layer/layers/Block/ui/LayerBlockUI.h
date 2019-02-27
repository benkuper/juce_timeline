/*
  ==============================================================================

    LayerBlockUI.h
    Created: 14 Feb 2019 11:14:58am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class LayerBlockUI :
	public BaseItemUI<LayerBlock>
{
public:
	LayerBlockUI(LayerBlock * block);
	~LayerBlockUI();

	//interaction
	float timeAtMouseDown;
	float coreLengthAtMouseDown;
	float loopLengthAtMouseDown;

	int posAtMouseDown;

	float viewStart;
	float viewEnd;
	float viewCoreEnd;

	virtual void paint(Graphics &g) override;

	virtual void resizedInternalHeader(Rectangle<int> &r) override;
	virtual void resizedInternalContent(Rectangle<int> &r) override;

	virtual void mouseEnter(const MouseEvent &e) override;
	virtual void mouseExit(const MouseEvent &e) override;
	virtual void mouseDown(const MouseEvent &e) override;
	virtual void mouseDrag(const MouseEvent &e) override;
	virtual void mouseUp(const MouseEvent &e) override;

	
	virtual void controllableFeedbackUpdateInternal(Controllable *) override;

	int getCoreWidth();

	void setViewRange(float relativeStart, float relativeEnd);
	virtual void setViewRangeInternal() {}

	/*
	class StretchHandle :
		public Component
	{
	public:
		StretchHandle(const Colour & c);
		~StretchHandle();
		
		Colour color;
		void paint(Graphics &g) override;
	};
	
	StretchHandle startHandle;
	StretchHandle coreHandle;
	StretchHandle loopHandle;
	*/

	Grabber coreGrabber;
	Grabber loopGrabber;

	class BlockUIListener
	{
	public:
		virtual ~BlockUIListener() {}
		virtual void blockUITimeChanged(LayerBlockUI *) {}
		virtual void blockUINeedsReorder() {}
		
		virtual void blockUIDragged(LayerBlockUI *, const MouseEvent &) {}
		virtual void blockUIStartDragged(LayerBlockUI *, const MouseEvent &) {}
		virtual void blockUICoreDragged(LayerBlockUI *, const MouseEvent &) {}
		virtual void blockUILoopDragged(LayerBlockUI *, const MouseEvent &) {}
	};

	ListenerList<BlockUIListener> blockUIListeners;
	void addBlockUIListener(BlockUIListener* newListener) { blockUIListeners.add(newListener); }
	void removeBlockUIListener(BlockUIListener* listener) { blockUIListeners.remove(listener); }
};