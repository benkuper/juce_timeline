/*
  ==============================================================================

	AudioLayerClipUI.cpp
	Created: 8 Feb 2017 12:20:09pm
	Author:  Ben

  ==============================================================================
*/

#include "JuceHeader.h"

#include <algorithm>
#include <memory>
#include <vector>

// Preview data is shared across timeline UIs, so closing a track does not stop a
// scan. Finished previews are also saved outside the project for the next launch.
namespace
{
constexpr int waveformSamplesPerPoint = 1024;
constexpr size_t maxLiveWaveforms = 32;
constexpr int maxSavedWaveforms = 256;

class WaveformFileSource : public FileInputSource
{
public:
    explicit WaveformFileSource(const File& file) : FileInputSource(file, true), sourceFile(file) {}

    int64 hashCode() const override
    {
        return FileInputSource::hashCode() ^ (static_cast<int64>(static_cast<uint64>(sourceFile.getSize()) * 0x5deece66dULL));
    }

private:
    File sourceFile;
};

class WaveformThumbnailCache : public AudioThumbnailCache
{
public:
    WaveformThumbnailCache() : AudioThumbnailCache(maxSavedWaveforms),
        directory(File::getSpecialLocation(File::userApplicationDataDirectory)
            .getChildFile("Chataigne").getChildFile("Waveforms")) {}

protected:
    bool loadNewThumb(AudioThumbnailBase& thumb, int64 hash) override
    {
        const File file = fileFor(hash);
        if (auto stream = std::unique_ptr<FileInputStream>(file.createInputStream()))
        {
            if (thumb.loadFrom(*stream) && thumb.isFullyLoaded() && thumb.getTotalLength() > 0)
                return true;
            file.deleteFile();
        }
        return false;
    }

    void saveNewlyFinishedThumbnail(const AudioThumbnailBase& thumb, int64 hash) override
    {
        if (!thumb.isFullyLoaded() || thumb.getTotalLength() <= 0) return;
        if (directory.createDirectory().failed()) return;

        TemporaryFile temporary(fileFor(hash));
        bool written = false;
        if (auto stream = std::unique_ptr<FileOutputStream>(temporary.getFile().createOutputStream()))
        {
            thumb.saveTo(*stream);
            stream->flush();
            written = stream->getStatus().wasOk();
        }
        if (!written || !temporary.overwriteTargetFileWithTemporary()) return;

        Array<File> files;
        directory.findChildFiles(files, File::findFiles, false, "*.thumb");
        if (files.size() <= maxSavedWaveforms) return;
        std::sort(files.begin(), files.end(), [](const File& a, const File& b)
        {
            return a.getLastModificationTime() < b.getLastModificationTime();
        });
        for (int i = 0; i < files.size() - maxSavedWaveforms; ++i)
            files[i].deleteFile();
    }

private:
    File fileFor(int64 hash) const
    {
        return directory.getChildFile("v1-1024-" + String::toHexString(hash) + ".thumb");
    }

    File directory;
};
}

struct WaveformThumbnail
{
    WaveformThumbnail(AudioFormatManager& formats, AudioThumbnailCache& cache, const File& sourceFile) :
        file(sourceFile), thumbnail(waveformSamplesPerPoint, formats, cache)
    {
        thumbnail.setSource(new WaveformFileSource(file));
    }

    File file;
    AudioThumbnail thumbnail;
};

namespace
{
class WaveformThumbnailStore
{
public:
    WaveformThumbnailStore()
    {
        formats.registerBasicFormats();
    }

    std::shared_ptr<WaveformThumbnail> get(const File& file)
    {
        if (!file.existsAsFile()) return {};

        const int64 hash = WaveformFileSource(file).hashCode();
        for (auto it = entries.begin(); it != entries.end(); ++it)
        {
            if (it->first == hash && it->second->file == file)
            {
                auto result = it->second;
                entries.erase(it);
                entries.emplace_back(hash, result);
                return result;
            }
        }

        auto result = std::make_shared<WaveformThumbnail>(formats, cache, file);
        entries.emplace_back(hash, result);
        while (entries.size() > maxLiveWaveforms)
        {
            auto unused = std::find_if(entries.begin(), entries.end(), [](const auto& entry)
            {
                return entry.second.use_count() == 1;
            });
            if (unused == entries.end()) break;
            entries.erase(unused);
        }
        return result;
    }

private:
    AudioFormatManager formats;
    WaveformThumbnailCache cache;
    std::vector<std::pair<int64, std::shared_ptr<WaveformThumbnail>>> entries;
};

WaveformThumbnailStore& waveformStore()
{
    static WaveformThumbnailStore store;
    return store;
}
}

AudioLayerClipUI::AudioLayerClipUI(AudioLayerClip* _clip) :
	LayerBlockUI(_clip),
	clip(_clip)
{
	dragAndDropEnabled = false;

	clip->addAsyncClipListener(this);
    clip->volume->addAsyncParameterListener(this);


#if JUCE_WINDOWS
	if (clip->filePath->stringValue().startsWithChar('/')) return;
#endif

}

AudioLayerClipUI::~AudioLayerClipUI()
{
	clearThumbnail();
	if (!inspectable.wasObjectDeleted()) { clip->removeAsyncClipListener(this); clip->volume->removeAsyncParameterListener(this); }
}

void AudioLayerClipUI::paint(Graphics& g)
{
	LayerBlockUI::paint(g);

	if (clip->filePath->stringValue().isEmpty()) return;
	g.setColour(Colours::white.withAlpha(.5f));
	if (clip->isLoading)
	{
		g.setFont(20);
		g.drawText("Loading...", getLocalBounds(), Justification::centred);

	}
	else
	{
		float volume = clip->volume->controlMode == Parameter::ControlMode::MANUAL ? clip->volume->floatValue() : 1;
		float stretch = clip->stretchFactor->floatValue();
		float startOffset = clip->clipStartOffset->floatValue();

		if (thumbnail != nullptr) thumbnail->thumbnail.drawChannels(g, getCoreBounds(), startOffset + viewStart / stretch, startOffset + viewCoreEnd / stretch, volume);
	}

    g.setColour(TEXT_COLOR);
    g.drawFittedText(clip->niceName, getLocalBounds().reduced(14, 3).withHeight(18), Justification::centredLeft, 1);
}

void AudioLayerClipUI::resizedBlockInternal() {}

void AudioLayerClipUI::addContextMenuItems(PopupMenu& menu)
{
    menu.addItem(1001, "Hide automation editor", automationUI != nullptr);
    menu.addItem(1002, "Edit volume automation");
    menu.addItem(1003, "Remove volume automation", clip->volume->controlMode != Parameter::MANUAL);
}

void AudioLayerClipUI::handleContextMenuResult(int result)
{
    if (result == 1001) setTargetAutomation(nullptr);
    else if (result == 1002)
    {
        if (clip->volume->controlMode != Parameter::AUTOMATION)
        {
            const float volume = clip->volume->floatValue();
            clip->volume->setControlMode(Parameter::AUTOMATION);
            clip->volume->automation->setManualMode(true);
            auto* a = dynamic_cast<Automation*>(clip->volume->automation->automationContainer);
            if (a)
            {
                a->clear(); a->setLength(clip->coreLength->floatValue());
                a->addKey(0, volume); a->addKey(a->length->floatValue(), volume);
            }
        }
        setTargetAutomation(clip->volume->automation.get());
    }
    else if (result == 1003)
    {
        setTargetAutomation(nullptr); clip->volume->setControlMode(Parameter::MANUAL);
    }
}

void AudioLayerClipUI::clearThumbnail()
{
	if (thumbnail != nullptr)
	{
		thumbnail->thumbnail.removeChangeListener(this);
		thumbnail.reset();
	}
}

void AudioLayerClipUI::setupThumbnail()
{
	clearThumbnail();
	thumbnail = waveformStore().get(clip->filePath->getFile());
	if (thumbnail != nullptr) thumbnail->thumbnail.addChangeListener(this);
	shouldRepaint = true;
}

void AudioLayerClipUI::setTargetAutomation(ParameterAutomation* a)
{
    setInlineEditor(a ? dynamic_cast<Automation*>(a->automationContainer) : nullptr,
        a ? dynamic_cast<GradientColorManager*>(a->automationContainer) : nullptr);
}


void AudioLayerClipUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	LayerBlockUI::controllableFeedbackUpdateInternal(c);

	if (c == item->time || c == item->coreLength || c == clip->fadeIn || c == clip->fadeOut)
	{
		shouldRepaint = true;
	}
	else if (c == clip->volume && clip->volume->controlMode == Parameter::ControlMode::MANUAL)
	{
		shouldRepaint = true;
	}

}

void AudioLayerClipUI::newMessage(const AudioLayerClip::ClipEvent& e)
{
	switch (e.type)
	{
	case AudioLayerClip::ClipEvent::SOURCE_LOAD_START:
		clearThumbnail();
		shouldRepaint = true;
		break;

	case AudioLayerClip::ClipEvent::SOURCE_LOAD_END:
		setupThumbnail();
		break;

	}
}

void AudioLayerClipUI::changeListenerCallback(ChangeBroadcaster* source)
{
	shouldRepaint = true;
}

void AudioLayerClipUI::newMessage(const Parameter::ParameterEvent& e)
{
    if (e.type == Parameter::ParameterEvent::CONTROLMODE_CHANGED && (automationUI || gradientUI))
        setTargetAutomation(clip->volume->automation.get());
}
