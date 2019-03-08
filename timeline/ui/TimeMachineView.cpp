/*
  ==============================================================================

    TimeMachineView.cpp
    Created: 28 Oct 2016 8:26:25pm
    Author:  bkupe

  ==============================================================================
*/

TimeMachineView::TimeMachineView(const String &contentName) :
	ShapeShifterContentComponent(contentName),
	autoSelectOnSequenceSelected(true)
{
	contentIsFlexible = true;
	InspectableSelectionManager::mainSelectionManager->addSelectionListener(this);

	Sequence * s = InspectableSelectionManager::mainSelectionManager->getInspectableAs<Sequence>();
	if (s != nullptr) setSequence(s);

	helpID = "SequenceEditor";

}

TimeMachineView::~TimeMachineView()
{
	if(InspectableSelectionManager::mainSelectionManager) InspectableSelectionManager::mainSelectionManager->removeSelectionListener(this);
	setSequence(nullptr);
}

void TimeMachineView::paint(Graphics & g)
{
	g.fillAll(BG_COLOR.darker(.3f));
	if (editor == nullptr)
	{
		g.setFont(20);
		g.setColour(BG_COLOR.brighter(.1f));
		g.drawFittedText("Create a sequence from the Sequences Panel to edit this sequence here.", getLocalBounds(), Justification::centred, 3);
	}
}

void TimeMachineView::resized()
{
	if (editor != nullptr)
	{
		editor->setBounds(getLocalBounds());
	}
}

void TimeMachineView::setSequence(Sequence * sequence)
{
	if (editor != nullptr && editor->sequence == sequence) return;

	if (editor != nullptr)
	{
		editor->sequence->removeInspectableListener(this);
		removeChildComponent(editor);
		editor = nullptr;
		
	}

	if (sequence != nullptr)
	{
		editor = new SequenceEditorView(sequence);
		editor->sequence->addInspectableListener(this);
		addAndMakeVisible(editor);
	}
	
	resized();
}

void TimeMachineView::inspectablesSelectionChanged()
{
	if (!autoSelectOnSequenceSelected) return;
	if (InspectableSelectionManager::mainSelectionManager->isEmpty()) return;
	
	Sequence * s = InspectableSelectionManager::mainSelectionManager->getInspectableAs<Sequence>();
	if (s != nullptr) setSequence(s);
}

void TimeMachineView::inspectableDestroyed(Inspectable * i)
{
	if (i == editor->sequence) setSequence(nullptr);
}
