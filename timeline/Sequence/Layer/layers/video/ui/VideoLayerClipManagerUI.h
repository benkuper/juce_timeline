/*
  ==============================================================================

    VideoLayerClipManagerUI.h
    Created: 26 Sep 2026

  ==============================================================================
*/

#pragma once

class VideoLayerTimeline;

class VideoLayerClipManagerUI :
	public LayerBlockManagerUI,
	public FileDragAndDropTarget
{
public:
	VideoLayerClipManagerUI(VideoLayerTimeline* timeline, VideoLayerClipManager* manager);
	~VideoLayerClipManagerUI();

	bool fileDropMode;

	virtual LayerBlockUI* createUIForItem(LayerBlock* item) override;

	void paintOverChildren(Graphics& g) override;

	void mouseDoubleClick(const MouseEvent& e) override;
	void addClipWithFileChooserAt(float position);

	// Inherited via FileDragAndDropTarget
	virtual bool isInterestedInFileDrag(const StringArray& files) override;
	virtual void fileDragEnter(const StringArray& files, int x, int y) override;
	virtual void fileDragMove(const StringArray& files, int x, int y) override;
	virtual void filesDropped(const StringArray& files, int x, int y) override;
};