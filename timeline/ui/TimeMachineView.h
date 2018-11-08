/*
  ==============================================================================

    TimeMachineView.h
    Created: 28 Oct 2016 8:26:25pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once


class TimeMachineView :
	public ShapeShifterContentComponent,
	public InspectableSelectionManager::Listener,
	public Inspectable::InspectableListener
{
public:
	
	TimeMachineView(const String &contentName);
	~TimeMachineView();

	ScopedPointer<SequenceEditorView> editor;
	
	void paint(Graphics &g) override;
	void resized() override;

	void setSequence(Sequence * sequence);
	virtual void inspectablesSelectionChanged() override;
	void inspectableDestroyed(Inspectable * i) override;


	static TimeMachineView * create(const String &contentName) { return new TimeMachineView(contentName); }


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeMachineView)
};


