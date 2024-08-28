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
	public InspectableSelectionManager::AsyncListener,
	public Inspectable::InspectableListener
{
public:
	
	TimeMachineView(const String &contentName);
	~TimeMachineView();

	std::unique_ptr<SequenceEditorView> editor;

	bool autoSelectOnSequenceSelected;
	static std::function<Sequence* ()> getEditingSequenceCustomFunc;
	
	void paint(Graphics &g) override;
	void resized() override;

	virtual void setSequence(Sequence * sequence);
	virtual SequenceEditorView * createEditorForSequence(Sequence * sequence);

	virtual void newMessage(const InspectableSelectionManager::SelectionEvent& e) override;
	void inspectableDestroyed(Inspectable * i) override;


	static TimeMachineView * create(const String &contentName) { return new TimeMachineView(contentName); }


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeMachineView)
};


