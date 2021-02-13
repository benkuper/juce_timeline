/*
  ==============================================================================

    AudioLayerClipUI.h
    Created: 8 Feb 2017 12:20:09pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class AudioLayerClipUI :
	public LayerBlockUI,
	public AudioLayerClip::AsyncListener
{
public:
	AudioLayerClipUI(AudioLayerClip * clip);
	~AudioLayerClipUI();

	AudioThumbnailCache thumbnailCache;
	AudioThumbnail thumbnail;
	AudioLayerClip * clip;

	std::unique_ptr<AutomationUI> automationUI;

	void paint(Graphics &g) override;

	void resizedBlockInternal() override;

	void mouseDown(const MouseEvent &e) override;


	virtual void setupThumbnail();

	void setTargetAutomation(ParameterAutomation* a);

	virtual void controllableFeedbackUpdateInternal(Controllable *) override;
	virtual void newMessage(const AudioLayerClip::ClipEvent &e) override;

};