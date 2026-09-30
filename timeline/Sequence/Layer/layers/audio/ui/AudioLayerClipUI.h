/*
  ==============================================================================

    AudioLayerClipUI.h
    Created: 8 Feb 2017 12:20:09pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

#include <memory>

struct WaveformThumbnail;

class AudioLayerClipUI :
	public LayerBlockUI,
	public AudioLayerClip::AsyncListener,
	public ChangeListener
{
public:
	AudioLayerClipUI(AudioLayerClip * clip);
	~AudioLayerClipUI();

	std::shared_ptr<WaveformThumbnail> thumbnail;
	AudioLayerClip * clip;

	std::unique_ptr<AutomationUI> automationUI;

	void paint(Graphics &g) override;

	void resizedBlockInternal() override;

	void mouseDown(const MouseEvent &e) override;

	virtual void setupThumbnail();
	void clearThumbnail();

	void setTargetAutomation(ParameterAutomation* a);

	virtual void controllableFeedbackUpdateInternal(Controllable *) override;
	virtual void newMessage(const AudioLayerClip::ClipEvent &e) override;

	virtual void changeListenerCallback(ChangeBroadcaster* source) override;
};