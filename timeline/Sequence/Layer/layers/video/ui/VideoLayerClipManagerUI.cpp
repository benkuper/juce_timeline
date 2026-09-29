/*
  ==============================================================================

    VideoLayerClipManagerUI.cpp
    Created: 26 Sep 2026

  ==============================================================================
*/

#include "../VideoFileHelpers.h"

VideoLayerClipManagerUI::VideoLayerClipManagerUI(VideoLayerTimeline* _timeline, VideoLayerClipManager* manager) :
	LayerBlockManagerUI(_timeline, manager),
	fileDropMode(false)
{
	addExistingItems();
}

VideoLayerClipManagerUI::~VideoLayerClipManagerUI()
{
}

void VideoLayerClipManagerUI::paintOverChildren(Graphics& g)
{
	LayerBlockManagerUI::paintOverChildren(g);

	if (fileDropMode)
	{
		g.fillAll(BLUE_COLOR.withAlpha(.2f));
		g.setColour(RED_COLOR);
		g.drawLine(getMouseXYRelative().x, 0, getMouseXYRelative().x, getHeight());
	}
}

LayerBlockUI* VideoLayerClipManagerUI::createUIForItem(LayerBlock* item)
{
	return new VideoLayerClipUI(dynamic_cast<VideoLayerClip*>(item));
}

void VideoLayerClipManagerUI::mouseDoubleClick(const MouseEvent& e)
{
	addClipWithFileChooserAt(getMouseXYRelative().x);
}

void VideoLayerClipManagerUI::addClipWithFileChooserAt(float position)
{
	FileChooser* chooser(new FileChooser("Load a video or image file", File::getCurrentWorkingDirectory(), VideoFileHelpers::getSupportedVideoAndImageWildcards()));
	chooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::FileChooserFlags::canSelectFiles, [this, position](const FileChooser& fc)
		{
			File f = fc.getResult();
			delete& fc;
			if (f == File()) return;

			float time = timeline->getTimeForX(position);
			VideoLayerClip* clip = dynamic_cast<VideoLayerClip*>(manager->addBlockAt(time));
			clip->filePath->setValue(f.getFullPathName());
		}
	);
}

bool VideoLayerClipManagerUI::isInterestedInFileDrag(const StringArray& files)
{
	if (files.size() == 0) return false;

	return VideoFileHelpers::isVideoOrImageFile(files[0]);
}

void VideoLayerClipManagerUI::fileDragEnter(const StringArray& files, int x, int y)
{
	fileDropMode = true;
	repaint();
}

void VideoLayerClipManagerUI::fileDragMove(const StringArray& files, int x, int y)
{
	repaint();
}

void VideoLayerClipManagerUI::filesDropped(const StringArray& files, int x, int y)
{
	if (files.size() == 0) return;

	float time = timeline->getTimeForX(getMouseXYRelative().x);
	VideoLayerClip* clip = dynamic_cast<VideoLayerClip*>(manager->addBlockAt(time));
	clip->filePath->setValue(files[0]);

	fileDropMode = false;
	repaint();
}