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
    public Parameter::AsyncListener,
	public ChangeListener
{
public:
	AudioLayerClipUI(AudioLayerClip * clip);
	~AudioLayerClipUI();

	std::shared_ptr<WaveformThumbnail> thumbnail;
	AudioLayerClip * clip;


	void paint(Graphics &g) override;

	void resizedBlockInternal() override;

	void addContextMenuItems(PopupMenu& menu) override;
	void handleContextMenuResult(int result) override;

	virtual void setupThumbnail();
	void clearThumbnail();

	void setTargetAutomation(ParameterAutomation* a);

	virtual void controllableFeedbackUpdateInternal(Controllable *) override;
	virtual void newMessage(const AudioLayerClip::ClipEvent &e) override;
    void newMessage(const Parameter::ParameterEvent& e) override;

	virtual void changeListenerCallback(ChangeBroadcaster* source) override;
};