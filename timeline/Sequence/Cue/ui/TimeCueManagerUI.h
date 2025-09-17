/*
  ==============================================================================

    TimeCueManagerUI.h
    Created: 6 Apr 2017 11:35:25am
    Author:  Ben

  ==============================================================================
*/


class SequenceTimelineHeader;

class TimeCueManagerUI :
	public ManagerUI<TimeCueManager, TimeCue, TimeCueUI>,
	public TimeCueUI::TimeCueUIListener
{
public:
	TimeCueManagerUI(SequenceTimelineHeader * header, TimeCueManager * manager);
	~TimeCueManagerUI();

	SequenceTimelineHeader * header;

	Array<float> snapTimes;

	void paint(Graphics& g) override;
	void resized() override;
	void updateContent();

	bool hitTest(int x, int y) override;

	void placeTimeCueUI(TimeCueUI *);

	void addCueAtPos(int x);

	void addItemFromMenu(bool, Point<int> mouseDownPos) override;
	void addBaseItemUIInternal(TimeCueUI * ttui) override;
	void removeBaseItemUIInternal(TimeCueUI * ttui) override;

	void cueMouseDown(TimeCueUI* ttui, const MouseEvent& e) override;
	void cueMouseUp(TimeCueUI* ttui, const MouseEvent& e) override;
	void cueDragged(TimeCueUI * ttui, const MouseEvent &e) override;
	void cueTimeChanged(TimeCueUI * ttui) override;
};
