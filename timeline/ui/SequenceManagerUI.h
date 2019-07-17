/*
  ==============================================================================

    SequenceManagerUI.h
    Created: 28 Oct 2016 8:13:29pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceManagerUI :
	public BaseManagerShapeShifterUI<SequenceManager, Sequence, SequenceUI>
{
public:
	SequenceManagerUI(const String &contentName, SequenceManager *_manager);
	~SequenceManagerUI();

	void mouseDoubleClick(const MouseEvent &e) override;

	static SequenceManagerUI * create(const String &contentName, SequenceManager * manager) { return new SequenceManagerUI(contentName, manager); }


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceManagerUI)
};
