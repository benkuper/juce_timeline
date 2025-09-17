/*
  ==============================================================================

    SequenceLayerTimeline.h
    Created: 28 Oct 2016 8:17:29pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SequenceLayerTimeline :
	public BaseItemMinimalUI<SequenceLayer>,
	public UITimerTarget
{
public:
	SequenceLayerTimeline(SequenceLayer *);
	virtual ~SequenceLayerTimeline();

    bool seekManipulationMode;
	
	int getXForTime(float time);
	float getTimeForX(int tx, bool offsetStart = true);
    
    void updateNeedlePosition();
	virtual void updateContent() {} // to be overriden
    virtual void updateMiniModeUI() {} //to be overriden;

    virtual void mouseDown(const MouseEvent &e) override;

    virtual void addSelectableComponentsAndInspectables(Array<Component*>& selectables, Array<Inspectable*>& inspectables) {}

    virtual void setSeekManipulationMode(bool isManipulating);

	void controllableFeedbackUpdateInternal(Controllable * c) override;


	void handlePaintTimerInternal() override;

	virtual void visibilityChanged() override;
    virtual void parentSizeChanged() override;
    
    class TimelineNeedle :
        public Component
    {
    public:
        TimelineNeedle();
        ~TimelineNeedle();
        
        const Colour defaultTimeBarColor = Colours::white.withAlpha(.4f);
        Colour timeBarColor;
        
        void paint(Graphics &g) override;
        
    };
    
    TimelineNeedle needle;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceLayerTimeline)
};

