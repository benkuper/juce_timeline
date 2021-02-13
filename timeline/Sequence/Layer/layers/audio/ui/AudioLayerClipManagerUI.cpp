/*
  ==============================================================================

    AudioLayerClipManagerUI.cpp
    Created: 8 Feb 2017 12:20:16pm
    Author:  Ben

  ==============================================================================
*/


AudioLayerClipManagerUI::AudioLayerClipManagerUI(AudioLayerTimeline * _timeline, AudioLayerClipManager * manager) :
	LayerBlockManagerUI(_timeline, manager),
	fileDropMode(false)
{
	addExistingItems();

}

AudioLayerClipManagerUI::~AudioLayerClipManagerUI()
{
}


void AudioLayerClipManagerUI::paintOverChildren(Graphics& g)
{
	LayerBlockManagerUI::paintOverChildren(g);

	if (fileDropMode)
	{
		g.fillAll(BLUE_COLOR.withAlpha(.2f));
		g.setColour(RED_COLOR);
		g.drawLine(getMouseXYRelative().x, 0, getMouseXYRelative().x, getHeight());
	}
}

LayerBlockUI* AudioLayerClipManagerUI::createUIForItem(LayerBlock* item)
{
	AudioLayerClipUI * ui =  new AudioLayerClipUI(dynamic_cast<AudioLayerClip *>(item));
	ui->setupThumbnail(); //do it from here to allow child override
	return ui;
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

bool AudioLayerClipManagerUI::isInterestedInFileDrag(const StringArray& files)
{
	if (files.size() == 0) return false;
	return  files[0].endsWith("mp3") || files[0].endsWith("wav") || files[0].endsWith("aiff");
}

void AudioLayerClipManagerUI::fileDragEnter(const StringArray& files, int x, int y)
{
	fileDropMode = true;
	repaint();
}

void AudioLayerClipManagerUI::fileDragMove(const StringArray& files, int x, int y)
{
	repaint();
}

void AudioLayerClipManagerUI::filesDropped(const StringArray& files, int x, int y)
{
	if (files.size() == 0) return;

	float time = timeline->getTimeForX(getMouseXYRelative().x);
	AudioLayerClip* clip = dynamic_cast<AudioLayerClip*>(manager->addBlockAt(time));
	clip->filePath->setValue(files[0]);

	fileDropMode = false;
	repaint();
}
