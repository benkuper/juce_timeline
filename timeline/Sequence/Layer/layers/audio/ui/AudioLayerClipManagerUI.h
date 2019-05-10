/*
  ==============================================================================

    AudioLayerClipManagerUI.h
    Created: 8 Feb 2017 12:20:16pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class AudioLayerTimeline;

class AudioLayerClipManagerUI :
	public BaseManagerUI<AudioLayerClipManager, AudioLayerClip, AudioLayerClipUI>,
	public AudioLayerClipUI::ClipUIListener
{
public:
	AudioLayerClipManagerUI(AudioLayerTimeline * timeline, AudioLayerClipManager * manager);
	~AudioLayerClipManagerUI();

	AudioLayerTimeline * timeline;

	void resized() override;
	void updateContent();

	void placeClipUI(AudioLayerClipUI * cui);
	void mouseDoubleClick(const MouseEvent &e) override;

	void addItemFromMenu(bool, Point<int> mouseDownPos) override;
	void addItemUIInternal(AudioLayerClipUI * cui) override;
	void removeItemUIInternal(AudioLayerClipUI * cui) override;

	void addClipWithFileChooserAt(float position);

	void clipUITimeChanged(AudioLayerClipUI * cui) override;
	void clipUIDragged(AudioLayerClipUI * cui, const MouseEvent &) override;
};