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
	AudioFormatManager formats;
	formats.registerBasicFormats();
	FileChooser* chooser(new FileChooser("Load an audio file", File::getCurrentWorkingDirectory(), formats.getWildcardForAllFormats()));
	Component::SafePointer<AudioLayerClipManagerUI> safeThis(this);
	chooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::FileChooserFlags::canSelectFiles, [safeThis, position](const FileChooser& fc)
		{
			File f = fc.getResult();
			delete& fc;
			if (f == File() || safeThis == nullptr) return;

			const float time = safeThis->timeline->getTimeForX(position);
			if (auto* clip = dynamic_cast<AudioLayerClip*>(safeThis->manager->addBlockAt(time)))
				clip->filePath->setValue(f.getFullPathName());
		}
	);
}

bool AudioLayerClipManagerUI::isInterestedInFileDrag(const StringArray& files)
{
	if (files.size() == 0) return false;
	const auto extension = File(files[0]).getFileExtension();
	AudioFormatManager formats;
	formats.registerBasicFormats();
	return formats.findFormatForFileExtension(extension) != nullptr;
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
