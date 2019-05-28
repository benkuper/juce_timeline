/*
  ==============================================================================

    TimeCueManagerUI.h
    Created: 6 Apr 2017 11:35:25am
    Author:  Ben

  ==============================================================================
*/

#pragma once

class SequenceTimelineHeader;

class TimeCueManagerUI :
	public BaseManagerUI<TimeCueManager, TimeCue, TimeCueUI>,
	public TimeCueUI::TimeCueUIListener
{
public:
	TimeCueManagerUI(SequenceTimelineHeader * header, TimeCueManager * manager);
	~TimeCueManagerUI();

	SequenceTimelineHeader * header;

	virtual void resized() override;
	virtual void updateContent();

	virtual bool hitTest(int x, int y) override;

	virtual void placeTimeCueUI(TimeCueUI *);

	virtual void addCueAtPos(int x);

	virtual void addItemFromMenu(bool, Point<int> mouseDownPos) override;
	virtual void addItemUIInternal(TimeCueUI * ttui) override;
	virtual void removeItemUIInternal(TimeCueUI * ttui) override;

	virtual void cueDragged(TimeCueUI * ttui, const MouseEvent &e) override;
	virtual void cueTimeChanged(TimeCueUI * ttui) override;
};