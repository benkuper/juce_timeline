/*
  ==============================================================================

    SequenceEditor.h
    Created: 28 Oct 2016 8:27:18pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceEditorView :
	public Component,
	public Sequence::SequenceListener,
	public ContainerAsyncListener,
	public ScrollBar::Listener,
	public GapGrabber::Listener,
	public SequenceTimelineSeeker::SeekerListener
{
public:
	SequenceEditorView(Sequence * _sequence, SequenceTimelineNavigationUI * navigationUI = nullptr, SequenceTransportUI * transportUI = nullptr) ;
	virtual ~SequenceEditorView();

	Sequence * sequence;
	WeakReference<Inspectable> sequenceRef;

	//ui
	Component panelContainer;
	Component timelineContainer;

	std::unique_ptr<SequenceTimelineNavigationUI> navigationUI;
	SequenceLayerPanelManagerUI panelManagerUI;
	SequenceLayerTimelineManagerUI timelineManagerUI;
	std::unique_ptr<SequenceTransportUI> transportUI;

	GapGrabber grabber;

	//layout
	float headerHeight = 60;
	float panelWidth;

	void paint(Graphics &g) override;
	void resized() override;
	
	// Inherited via Listener
	virtual void scrollBarMoved(ScrollBar * scrollBarThatHasMoved, double newRangeStart) override;

	void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &) override;
	void mouseMagnify(const MouseEvent& e, float scaleFactor) override;
	bool keyPressed(const KeyPress &key) override;

	void seekerManipulationChanged(bool isManipulating) override;

	void newMessage(const ContainerAsyncEvent& e) override;

	// Inherited via Listener
	virtual void grabberGrabUpdate(GapGrabber *, int relativeDist) override;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceEditorView)
};