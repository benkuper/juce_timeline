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
	public LayerBlockManagerUI,
	public FileDragAndDropTarget
{
public:
	AudioLayerClipManagerUI(AudioLayerTimeline * timeline, AudioLayerClipManager * manager);
	~AudioLayerClipManagerUI();

	bool fileDropMode;

	virtual LayerBlockUI* createUIForItem(LayerBlock* item) override;

	void paintOverChildren(Graphics& g) override;


	void mouseDoubleClick(const MouseEvent &e) override;
	void addClipWithFileChooserAt(float position);

	// Inherited via FileDragAndDropTarget
	virtual bool isInterestedInFileDrag(const StringArray& files) override;
	virtual void fileDragEnter(const StringArray& files, int x, int y) override;
	virtual void fileDragMove(const StringArray& files, int x, int y) override;
	virtual void filesDropped(const StringArray& files, int x, int y) override;

};