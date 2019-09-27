#include "AudioLayerClipManagerUI.h"
/*
  ==============================================================================

    AudioLayerClipManagerUI.cpp
    Created: 8 Feb 2017 12:20:16pm
    Author:  Ben

  ==============================================================================
*/


AudioLayerClipManagerUI::AudioLayerClipManagerUI(AudioLayerTimeline * _timeline, AudioLayerClipManager * manager) :
	LayerBlockManagerUI(_timeline, manager)
{
	addExistingItems();

}

AudioLayerClipManagerUI::~AudioLayerClipManagerUI()
{
}


LayerBlockUI* AudioLayerClipManagerUI::createUIForItem(LayerBlock* item)
{
	return new AudioLayerClipUI(dynamic_cast<AudioLayerClip *>(item));
}

void AudioLayerClipManagerUI::mouseDoubleClick(const MouseEvent & e)
{
	addClipWithFileChooserAt(getMouseXYRelative().x);
}

void AudioLayerClipManagerUI::addClipWithFileChooserAt(float position)
{
	FileChooser chooser("Load an audio file", File(), "*.wav;*.mp3;*.ogg;*.aiff");
	bool result = chooser.browseForFileToOpen();
	if (result)
	{
		float time = timeline->getTimeForX(position);
		AudioLayerClip* clip = dynamic_cast<AudioLayerClip*>(manager->addBlockAt(time));
		clip->filePath->setValue(chooser.getResult().getFullPathName());
	}
}