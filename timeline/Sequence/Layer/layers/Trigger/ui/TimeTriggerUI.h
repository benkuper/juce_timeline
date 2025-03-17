/*
  ==============================================================================

    TimeTriggerUI.h
    Created: 10 Dec 2016 11:57:16am
    Author:  Ben

  ==============================================================================
*/

#pragma once

class TimeTriggerUI :
	public BaseItemUI<TimeTrigger>
{
public:
	TimeTriggerUI(TimeTrigger * tt);
	~TimeTriggerUI();
	
	//layout
	Rectangle<int> flagRect;
	Rectangle<int> lineRect;
	Rectangle<int> lengthRect;

	std::unique_ptr<BoolToggleUI> lockUI;

	//interaction
	float timeAtMouseDown;
	float lengthAtMouseDown;
	float flagYAtMouseDown;
	bool draggingLength;

	int startXOffset;
	int flagXOffset; //to avoid end of layer overflow
	int labelWidth;
	int triggerWidth;

	void paint(Graphics &g) override;
	void resized() override;

	bool hitTest(int x, int y) override;

	void updateSizeFromName();

	void mouseDown(const MouseEvent &e) override;
	void mouseDrag(const MouseEvent &e) override;
	void mouseUp(const MouseEvent &e) override;
	void mouseMove(const MouseEvent &e) override;

	void containerChildAddressChangedAsync(ControllableContainer *) override;
	void controllableFeedbackUpdateInternal(Controllable *) override;
	void inspectableSelectionChanged(Inspectable *) override;
	
	class TimeTriggerUIListener
	{
	public:
		virtual ~TimeTriggerUIListener() {}
		virtual void timeTriggerTimeChanged(TimeTriggerUI *) {}
		virtual void timeTriggerMouseDown(TimeTriggerUI*, const MouseEvent&) {}
		virtual void timeTriggerDragged(TimeTriggerUI*, const MouseEvent&) {}
		virtual void timeTriggerDeselected(TimeTriggerUI *) {}
	};

	ListenerList<TimeTriggerUIListener> triggerUIListeners;
	void addTriggerUIListener(TimeTriggerUIListener* newListener) { triggerUIListeners.add(newListener); }
	void removeTriggerUIListener(TimeTriggerUIListener* listener) { triggerUIListeners.remove(listener); }

};