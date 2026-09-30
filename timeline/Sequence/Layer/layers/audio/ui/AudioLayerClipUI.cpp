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
	bgColor = clip->isActive->boolValue() ? AUDIO_COLOR.brighter() : BG_COLOR.brighter(.1f);

	clip->addAsyncClipListener(this);


#if JUCE_WINDOWS
	if (clip->filePath->stringValue().startsWithChar('/')) return;
#endif

}

AudioLayerClipUI::~AudioLayerClipUI()
{
	clearThumbnail();
	if (!inspectable.wasObjectDeleted()) clip->removeAsyncClipListener(this);
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
		float viewRange = viewCoreEnd - viewStart;

		if (thumbnail != nullptr) thumbnail->thumbnail.drawChannels(g, getCoreBounds(), startOffset + viewStart, startOffset + viewStart + viewRange / stretch, volume);
	}

	if (clip->fadeIn->floatValue() > 0)
	{
		g.setColour(YELLOW_COLOR.withAlpha(.2f));
		int fadeInWidth = clip->fadeIn->floatValue() * getCoreWidth() / clip->coreLength->floatValue();
		Path p;
		p.startNewSubPath(0, 0);
		p.lineTo(fadeInWidth, 0);
		p.lineTo(0, getHeight());
		p.closeSubPath();
		g.fillPath(p);
	}

	if (clip->fadeOut->floatValue() > 0)
	{
		g.setColour(YELLOW_COLOR.withAlpha(.2f));
		int fadeOutWidth = clip->fadeOut->floatValue() * getCoreWidth() / clip->coreLength->floatValue();
		Path p;
		p.startNewSubPath(getCoreWidth(), 0);
		p.lineTo(getCoreWidth() - fadeOutWidth, 0);
		p.lineTo(getCoreWidth(), getHeight());
		p.closeSubPath();
		g.fillPath(p);
	}

	if (automationUI != nullptr)
	{
		Rectangle<int> r = getCoreBounds();
		if (automationUI != nullptr)
		{
			/*if (dynamic_cast<GradientColorManagerUI*>(automationUI.get()) != nullptr) automationUI->setBounds(r.removeFromBottom(20));
			else
			*/
			automationUI->setBounds(r);
		}
	}
}

void AudioLayerClipUI::resizedBlockInternal()
{
}

void AudioLayerClipUI::mouseDown(const MouseEvent& e)
{
	LayerBlockUI::mouseDown(e);
	if (e.mods.isRightButtonDown() && (e.eventComponent == this || e.eventComponent == &automationUI->keysUI))
	{
		PopupMenu::dismissAllActiveMenus();
		PopupMenu p;
		p.addItem(1, "Clear automation editor", automationUI != nullptr);
		p.addItem(2, "Edit enveloppe automation", automationUI == nullptr);
		p.addItem(3, "Remove enveloppe automation", clip->volume->controlMode != Parameter::ControlMode::MANUAL);
		if (automationUI != nullptr)
		{
			p.addSeparator();
			automationUI->keysUI.addMenuExtraItems(p, 4);
		}

		p.showMenuAsync(PopupMenu::Options(), [this](int result)
			{
				AudioLayerClip* clip = this->clip;

				switch (result)
				{
				case 1:
					this->setTargetAutomation(nullptr);
					break;

				case 2:
				{
					if (clip->volume->controlMode != Parameter::ControlMode::AUTOMATION)
					{
						clip->volume->setControlMode(Parameter::ControlMode::AUTOMATION);
						clip->volume->automation->setManualMode(true);

						Automation* a = dynamic_cast<Automation*>(clip->volume->automation->automationContainer);

						if (a != nullptr)
						{
							a->clear();
							a->setLength(clip->coreLength->floatValue());
							AutomationKey* k = a->addItem(0, 0);
							k->setEasing(Easing::BEZIER);
							a->addKey(a->length->floatValue(), 1);
						}
					}

					this->setTargetAutomation(clip->volume->automation.get());
				}
				break;

				case 3:
					this->setTargetAutomation(nullptr);
					clip->volume->setControlMode(Parameter::ControlMode::MANUAL);
					break;

				default:
					if (result > 3)
						automationUI->keysUI.handleMenuExtraItemsResult(result, 4);
					break;
				}
			}
		);
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
	if (automationUI != nullptr)
	{
		removeChildComponent(automationUI.get());
		automationUI = nullptr;
	}

	canBeGrabbed = true;

	if (a == nullptr) return;


	if (dynamic_cast<ParameterNumberAutomation*>(a) != nullptr)
	{
		AutomationUI* aui = new AutomationUI((Automation*)a->automationContainer);
		//aui->updateROI();
		aui->showMenuOnRightClick = false;
		automationUI.reset(aui);
	}
	/*
	else if (dynamic_cast<ParameterColorAutomation*>(a) != nullptr)
	{
		GradientColorManagerUI* gui = new GradientColorManagerUI((GradientColorManager*)a->automationContainer);
		gui->autoResetViewRangeOnLengthUpdate = true;
		automationUI.reset(gui);
	}
	*/

	if (automationUI != nullptr)
	{
		canBeGrabbed = false;
		coreGrabber.setVisible(false);
		grabber.setVisible(false);
		loopGrabber.setVisible(false);
		automationUI->keysUI.addMouseListener(this, false);
		addAndMakeVisible(automationUI.get());
		resized();
	}
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

	if (c == clip->isActive)
	{
		bgColor = clip->isActive->boolValue() ? AUDIO_COLOR.brighter() : BG_COLOR.brighter(.1f);
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
