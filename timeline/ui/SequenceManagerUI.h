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

	std::unique_ptr<TriggerImageUI> playAllUI;
	std::unique_ptr<TriggerImageUI> stopAllUI;

	void resizedInternalHeader(Rectangle<int>& r) override;
	void mouseDoubleClick(const MouseEvent &e) override;

	void addMenuExtraItems(juce::PopupMenu& p, int startIndex) override;


#if TIMELINE_USE_SEQUENCEMANAGER_SINGLETON
	static SequenceManagerUI* create(const String& contentName) { return new SequenceManagerUI(contentName, SequenceManager::getInstance()); }
#else
	static SequenceManagerUI* create(const String& contentName, SequenceManager* manager) { return new SequenceManagerUI(contentName, manager); }

#endif


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceManagerUI)
};
