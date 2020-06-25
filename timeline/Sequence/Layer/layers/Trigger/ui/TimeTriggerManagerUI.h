/*
  ==============================================================================

    TimeTriggerManagerUI.h
    Created: 10 Dec 2016 12:23:33pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class TimeTriggerManagerUI :
	public BaseManagerUI<TimeTriggerManager, TimeTrigger, TimeTriggerUI>,
	public TimeTriggerUI::TimeTriggerUIListener,
	public InspectableSelectionManager::Listener,
	public InspectableSelector::SelectorListener
{
public:
	TimeTriggerManagerUI(TriggerLayerTimeline * timeline, TimeTriggerManager * manager);
	~TimeTriggerManagerUI();

	TriggerLayerTimeline * timeline;
	std::unique_ptr<TimeTriggerMultiTransformer> transformer;

	bool miniMode;

	void setMiniMode(bool value);

	void resized() override;
	void updateContent();

	void placeTimeTriggerUI(TimeTriggerUI *);

	void mouseDown(const MouseEvent &e) override;
	void mouseDoubleClick(const MouseEvent &e) override;

	void addItemFromMenu(bool, Point<int> mouseDownPos) override;
	void addItemUIInternal(TimeTriggerUI * ttui) override;
	void removeItemUIInternal(TimeTriggerUI * ttui) override;

	void timeTriggerDragged(TimeTriggerUI * ttui, const MouseEvent &e) override;
	void timeTriggerTimeChanged(TimeTriggerUI * ttui) override;


	void selectionEnded(Array<Component *> selectedComponents) override;
};