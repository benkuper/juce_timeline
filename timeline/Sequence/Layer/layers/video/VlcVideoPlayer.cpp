/*
  ==============================================================================

    VlcVideoPlayer.cpp
    Created: 27 Sep 2026

    libVLC-backed video playback. libVLC is loaded dynamically at runtime from
    the installed VLC so the app doesn't need a link-time dependency.

    Frames are decoded to RGBA by libVLC and copied into a JUCE Image using the
    callback renderer - no native child window is involved, which makes the
    component safe to embed into any panel/scroll area.

  ==============================================================================
*/

#include "JuceHeader.h"
#include "VlcVideoPlayer.h"
#include "VideoFileHelpers.h"

#include <functional>
#include <atomic>
#include <cstdlib>
#include <cstring>

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace
{
	//==============================================================================
	// libVLC opaque handles
	typedef void* LibVlcMedia;
	typedef void* LibVlcMediaPlayer;
	typedef void* LibVlcEventManager;

	// libvlc states
	enum
	{
		vlcStateNothingSpecial = 0,
		vlcStateOpening = 1,
		vlcStateBuffering = 2,
		vlcStatePlaying = 3,
		vlcStatePaused = 4,
		vlcStateStopped = 5,
		vlcStateEnded = 6,
		vlcStateError = 7
	};

	// libvlc events
	enum
	{
		vlcEventMediaPlayerPaused = 0x105,
		vlcEventMediaPlayerStopped = 0x106,
		vlcEventMediaPlayerEndReached = 0x109
	};

	//==============================================================================
	struct LibVCLLibrary
	{
		DynamicLibrary module;
		void* instance = nullptr;
		bool attempted = false;

		void* (*new_instance)(int argc, const char* const* argv) = nullptr;
		void (*release_instance)(void*) = nullptr;
		const char* (*get_version)() = nullptr;

		LibVlcMedia (*media_new_path)(void*, const char*) = nullptr;
		void (*media_release)(LibVlcMedia) = nullptr;
		int (*media_add_option)(LibVlcMedia, const char*) = nullptr;
		int (*media_parse_with_options)(LibVlcMedia, int, int) = nullptr;
		long long (*media_get_duration)(LibVlcMedia) = nullptr;

		LibVlcMediaPlayer (*player_new_from_media)(LibVlcMedia) = nullptr;
		void (*player_release)(LibVlcMediaPlayer) = nullptr;
		int (*player_play)(LibVlcMediaPlayer) = nullptr;
		void (*player_set_pause)(LibVlcMediaPlayer, int) = nullptr;
		void (*player_stop)(LibVlcMediaPlayer) = nullptr;
		int (*player_set_time)(LibVlcMediaPlayer, long long) = nullptr;
		long long (*player_get_time)(LibVlcMediaPlayer) = nullptr;
		long long (*player_get_length)(LibVlcMediaPlayer) = nullptr;
		int (*player_set_rate)(LibVlcMediaPlayer, float) = nullptr;
		int (*player_get_state)(LibVlcMediaPlayer) = nullptr;
		LibVlcEventManager (*player_event_manager)(LibVlcMediaPlayer) = nullptr;

		int (*event_attach)(LibVlcEventManager, int, void*, void*) = nullptr;
		int (*event_detach)(LibVlcEventManager, int, void*, void*) = nullptr;

		void (*video_set_callbacks)(LibVlcMediaPlayer, void*, void*, void*, void*) = nullptr;
		void (*video_set_format_callbacks)(LibVlcMediaPlayer, void*, void*) = nullptr;
		int (*audio_set_volume)(LibVlcMediaPlayer, int) = nullptr;

		//==============================================================================
		~LibVCLLibrary()
		{
			if (instance != nullptr && release_instance != nullptr)
				release_instance(instance);
		}

		bool isReady() const { return module.getNativeHandle() != nullptr && instance != nullptr; }

		template <class T> bool loadSymbol(T& target, const char* name)
		{
			target = reinterpret_cast<T>(module.getFunction(name));
			return target != nullptr;
		}

		bool loadAllSymbols()
		{
			return loadSymbol(new_instance, "libvlc_new")
				&& loadSymbol(release_instance, "libvlc_release")
				&& loadSymbol(get_version, "libvlc_get_version")
				&& loadSymbol(media_new_path, "libvlc_media_new_path")
				&& loadSymbol(media_release, "libvlc_media_release")
				&& loadSymbol(media_add_option, "libvlc_media_add_option")
				&& loadSymbol(media_parse_with_options, "libvlc_media_parse_with_options")
				&& loadSymbol(media_get_duration, "libvlc_media_get_duration")
				&& loadSymbol(player_new_from_media, "libvlc_media_player_new_from_media")
				&& loadSymbol(player_release, "libvlc_media_player_release")
				&& loadSymbol(player_play, "libvlc_media_player_play")
				&& loadSymbol(player_set_pause, "libvlc_media_player_set_pause")
				&& loadSymbol(player_stop, "libvlc_media_player_stop")
				&& loadSymbol(player_set_time, "libvlc_media_player_set_time")
				&& loadSymbol(player_get_time, "libvlc_media_player_get_time")
				&& loadSymbol(player_get_length, "libvlc_media_player_get_length")
				&& loadSymbol(player_set_rate, "libvlc_media_player_set_rate")
				&& loadSymbol(player_get_state, "libvlc_media_player_get_state")
				&& loadSymbol(player_event_manager, "libvlc_media_player_event_manager")
				&& loadSymbol(event_attach, "libvlc_event_attach")
				&& loadSymbol(event_detach, "libvlc_event_detach")
				&& loadSymbol(video_set_callbacks, "libvlc_video_set_callbacks")
				&& loadSymbol(video_set_format_callbacks, "libvlc_video_set_format_callbacks")
				&& loadSymbol(audio_set_volume, "libvlc_audio_set_volume");
		}

		bool ensureLoaded()
		{
			if (attempted) return isReady();
			attempted = true;

			const char* envDir = std::getenv("CHATAIGNE_VLC_DIR");
			StringArray searchDirectories;
			StringArray libraryNames;

			// The bundled runtime ships next to the executable : that takes priority so the
			// app doesn't depend on VLC being installed on the machine.
			const File executable = File::getSpecialLocation(File::currentExecutableFile);
			const File application = File::getSpecialLocation(File::currentApplicationFile);
			searchDirectories.add(executable.getParentDirectory().getFullPathName());
			searchDirectories.add(executable.getParentDirectory().getChildFile("lib").getFullPathName());
			searchDirectories.add(application.getParentDirectory().getFullPathName());
			searchDirectories.add(File::getCurrentWorkingDirectory().getFullPathName());

			if (envDir != nullptr && envDir[0] != 0)
				searchDirectories.add(envDir);

#if JUCE_WINDOWS
			libraryNames.add("libvlc.dll");
			searchDirectories.add("C:/Program Files/VideoLAN/VLC");
			searchDirectories.add("C:/Program Files (x86)/VideoLAN/VLC");
#elif JUCE_MAC
			libraryNames.add("libvlc.dylib");
			if (application.hasFileExtension("app"))
			{
				searchDirectories.add(application.getChildFile("Contents/Frameworks").getFullPathName());
				searchDirectories.add(application.getChildFile("Contents/MacOS/lib").getFullPathName());
			}
			searchDirectories.add("/Applications/VLC.app/Contents/MacOS/lib");
			searchDirectories.add("/opt/homebrew/lib");
			searchDirectories.add("/usr/local/lib");
			searchDirectories.add("/opt/local/lib");
#else
			// Try the versioned Debian/Ubuntu soname first, then the development symlink.
			libraryNames.add("libvlc.so.5");
			libraryNames.add("libvlc.so");
#endif

			searchDirectories.removeDuplicates(false);

			StringArray candidates;
			for (const auto& directory : searchDirectories)
				for (const auto& libraryName : libraryNames)
					candidates.add(File(directory).getChildFile(libraryName).getFullPathName());

#if ! JUCE_WINDOWS
			// Let the platform loader search its configured paths after explicit bundled
			// and application locations have been tried.
			candidates.addArray(libraryNames);
#endif

			for (const auto& candidate : candidates)
			{
#if JUCE_WINDOWS
				const String dependencyDirectory = File(candidate).getParentDirectory().getFullPathName();
				SetDllDirectoryW(dependencyDirectory.toWideCharPointer());
#endif
				const bool opened = module.open(candidate);
#if JUCE_WINDOWS
				SetDllDirectoryW(nullptr);
#endif
				if (!opened)
					continue;

				if (!loadAllSymbols())
				{
					module.close();
					continue;
				}

				const char* mainArgv0 = "chataigne";
				const char* mainArgv1 = "--no-video-title-show";
				const char* mainArgv2 = "--no-osd";
				const char* mainArgv3 = "--quiet";

				const char* const mainArgv[] = { mainArgv0, mainArgv1, mainArgv2, mainArgv3 };
				instance = new_instance(4, mainArgv);

				if (instance == nullptr)
				{
					module.close();
					continue;
				}

				if (get_version != nullptr)
					Logger::writeToLog("Video backend: libVLC " + String(get_version()) + " (" + candidate + ")");

				return true;
			}

			Logger::writeToLog("Video backend: could not load libVLC (is VLC installed? Set CHATAIGNE_VLC_DIR to override)");
			return false;
		}

		static LibVCLLibrary& get()
		{
			static LibVCLLibrary library;
			return library;
		}
	};
}

//==============================================================================
// libVLC callbacks, called from libVLC's internal threads
//==============================================================================

struct VlcVideoPlayer::FrameStore
{
	unsigned videoW = 0;
	unsigned videoH = 0;
	void* buffer = nullptr;
	Image images[2];
	std::atomic<int> current { 0 };

	~FrameStore()
	{
		reset();
	}

	void reset()
	{
		std::free(buffer);
		buffer = nullptr;
		videoW = videoH = 0;
		images[0] = Image();
		images[1] = Image();
		current.store(0);
	}

	Image getFront() const
	{
		return images[current.load()];
	}

	void publishFrame()
	{
		if (buffer == nullptr || videoW == 0 || videoH == 0) return;

		int back = 1 - current.load();

		if (images[back].isValid())
		{
			Image::BitmapData bd(images[back], Image::BitmapData::writeOnly);
			const uint8* src = static_cast<const uint8*>(buffer);
			const size_t srcPitch = (size_t) videoW * 4;

			for (unsigned y = 0; y < videoH; ++y)
			{
				uint8* dst = bd.getLinePointer((int) y);
				const uint8* srcRow = src + y * srcPitch;

				// libVLC hands the pixels as straight (non-premultiplied) alpha in
				// B,G,R,A byte order. JUCE Image::ARGB expects PREMULTIPLIED alpha,
				// otherwise semi-transparent pixels (alpha edges, alpha video) get
				// composited too dark and transparency comes out wrong.
				for (unsigned x = 0; x < videoW; ++x)
				{
					const uint8* s = srcRow + x * 4;
					uint8* d = dst + x * 4;

					const unsigned a = s[3];
					d[0] = (uint8) ((s[0] * a) / 255);
					d[1] = (uint8) ((s[1] * a) / 255);
					d[2] = (uint8) ((s[2] * a) / 255);
					d[3] = (uint8) a;
				}
			}

			current.store(back);
		}
	}
};

unsigned vlcFormatSetup(void** opaque, char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines)
{
	auto* self = static_cast<VlcVideoPlayer*>(*opaque);

	if (chroma != nullptr)
	{
		// Ask libVLC for B,G,R,A byte order : our target is a JUCE Image::ARGB,
		// whose in-memory pixel layout is BGR(A). Requesting "RGBA" instead would
		// leave R and B swapped (blue faces on photos).
		chroma[0] = 'B';
		chroma[1] = 'G';
		chroma[2] = 'R';
		chroma[3] = 'A';
		chroma[4] = 0;
	}

	self->frameStore->reset();

	if (width != nullptr && height != nullptr)
	{
		self->frameStore->videoW = *width;
		self->frameStore->videoH = *height;
		self->frameStore->buffer = std::malloc((size_t) *width * *height * 4);
	}

	if (pitches != nullptr)  *pitches = *width * 4;
	if (lines != nullptr)    *lines = *height;

	if (self->frameStore->buffer != nullptr)
	{
		self->frameStore->images[0] = Image(Image::ARGB, (int) *width, (int) *height, true);
		self->frameStore->images[1] = Image(Image::ARGB, (int) *width, (int) *height, true);
		return 1;
	}

	return 0;
}

void vlcCleanup(void* opaque)
{
	auto* self = static_cast<VlcVideoPlayer*>(opaque);
	self->frameStore->reset();
}

void* vlcLock(void* opaque, void** planes)
{
	auto* self = static_cast<VlcVideoPlayer*>(opaque);
	planes[0] = self->frameStore->buffer;
	return planes[0];
}

void vlcUnlock(void*, void*, void* const*)
{
}

void vlcDisplay(void* opaque, void*)
{
	auto* self = static_cast<VlcVideoPlayer*>(opaque);
	self->frameStore->publishFrame();
	self->hasNewFrame.store(true);
	self->triggerAsyncUpdate();
}

void vlcEventCallback(const void* evtPtr, void* opaque)
{
	auto* self = static_cast<VlcVideoPlayer*>(opaque);

	if (evtPtr == nullptr) return;

	const int eventType = *static_cast<const int*>(evtPtr);

	if (eventType == vlcEventMediaPlayerEndReached
		|| eventType == vlcEventMediaPlayerPaused
		|| eventType == vlcEventMediaPlayerStopped)
	{
		self->needsStoppedEvent.store(true);
		self->triggerAsyncUpdate();
	}
}

//==============================================================================
// VlcVideoPlayer
//==============================================================================

VlcVideoPlayer::VlcVideoPlayer()
{
	frameStore.reset(new FrameStore());
	setOpaque(true);
}

VlcVideoPlayer::~VlcVideoPlayer()
{
	closeVideo();
}

Result VlcVideoPlayer::load(const File& file)
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();

	// Already open for that exact file (case-insensitive on Windows) : nothing to do.
	// This must never reopen a running player just because syncPlaybackState re-runs.
	if (player != nullptr && media != nullptr && videoLoaded && currentFile == file)
	{
		lastLoadWasReuse = true;
		return Result::ok();
	}

	lastLoadWasReuse = false;
	closeVideo();

	if (!vlc.ensureLoaded() || !vlc.isReady())
		return Result::fail("VLC library not found");

	media = vlc.media_new_path(vlc.instance, file.getFullPathName().toUTF8());

	if (media == nullptr)
		return Result::fail("VLC could not open this file");

	const bool isStillImage = VideoFileHelpers::isStillImageFile(file.getFullPathName());

	if (isStillImage && vlc.media_add_option != nullptr)
	{
		// A still image otherwise plays one frame and immediately goes to "ended",
		// which would make the layer resync-loop while the sequence plays.
		vlc.media_add_option(media, "input-repeat=65535");
	}

	player = vlc.player_new_from_media(media);

	if (player == nullptr)
	{
		vlc.media_release(media);
		media = nullptr;
		return Result::fail("VLC could not create a player for this file");
	}

	eventManager = vlc.player_event_manager(player);

	if (eventManager != nullptr)
	{
		vlc.event_attach(eventManager, vlcEventMediaPlayerEndReached, (void*) &vlcEventCallback, this);
		vlc.event_attach(eventManager, vlcEventMediaPlayerPaused, (void*) &vlcEventCallback, this);
		vlc.event_attach(eventManager, vlcEventMediaPlayerStopped, (void*) &vlcEventCallback, this);
	}

	vlc.video_set_callbacks(player, (void*) &vlcLock, (void*) &vlcUnlock, (void*) &vlcDisplay, this);
	vlc.video_set_format_callbacks(player, (void*) &vlcFormatSetup, (void*) &vlcCleanup);

	currentFile = file;
	videoLoaded = true;
	hasNewFrame.store(false);
	needsStoppedEvent.store(false);

	// Start playback : the player needs a moment to open the input before the length
	// is known, so poll until the length is valid. libvlc_media_player_get_length is the
	// authoritative source : libvlc_media_get_duration is unreliable on some files (it can
	// stay -1, or worse, return a positive but wrong value) so it is only a fallback.
	vlc.audio_set_volume(player, 100);
	vlc.player_play(player);

	int state = vlcStateNothingSpecial;
	long long mediaDur = vlc.media_get_duration(media);
	durationMs = vlc.player_get_length(player);

	if (isStillImage)
	{
		// Still images never report a meaningful length : give the decoder a
		// moment to produce the frame, then park. No 3s length-polling stall.
		Thread::sleep(200);
		state = vlc.player_get_state(player);
	}
	else
	{
		for (int i = 0; i < 30; ++i)
		{
			state = vlc.player_get_state(player);

			const long long len = vlc.player_get_length(player);
			if (len > 0) durationMs = len;

			if ((state == vlcStatePlaying || state == vlcStatePaused) && durationMs > 0)
				break;

			if (state == vlcStateEnded || state == vlcStateError)
				break;

			Thread::sleep(100);
		}

		// If the player never produced a length, fall back to the container guess.
		if (durationMs <= 0) durationMs = mediaDur;
	}

	durationMs = durationMs < 0 ? 0 : durationMs;

	if (juce::Logger::getCurrentLogger() != nullptr)
		Logger::writeToLog("VLC load: state=" + String(state)
			+ " mediaDur=" + String(mediaDur)
			+ " playerLen=" + String((long long) vlc.player_get_length(player))
			+ " finalDur=" + String(durationMs)
			+ " isImage=" + String(isStillImage ? 1 : 0));

	if (!isStillImage && (state == vlcStateError || (state == vlcStateEnded && durationMs <= 0)))
	{
		// Could not decode the file : clean up and report an error.
		closeVideo();
		return Result::fail("VLC could not decode this video");
	}

	// Park on the first frame so a static image shows while the sequence is stopped.
	// (set_pause/seek are only called once the player is actually started).
	vlc.player_set_pause(player, 1);
	vlc.player_set_time(player, 0);

	return Result::ok();
}

void VlcVideoPlayer::closeVideo()
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();

	if (player != nullptr && vlc.isReady())
	{
		if (eventManager != nullptr)
		{
			vlc.event_detach(eventManager, vlcEventMediaPlayerEndReached, (void*) &vlcEventCallback, this);
			vlc.event_detach(eventManager, vlcEventMediaPlayerPaused, (void*) &vlcEventCallback, this);
			vlc.event_detach(eventManager, vlcEventMediaPlayerStopped, (void*) &vlcEventCallback, this);
		}

		vlc.player_stop(player);
		vlc.player_release(player);
		player = nullptr;
	}

	if (media != nullptr && vlc.isReady())
	{
		vlc.media_release(media);
		media = nullptr;
	}

	if (frameStore != nullptr)
		frameStore->reset();

	videoLoaded = false;
	currentFile = File();
	durationMs = 0;
	eventManager = nullptr;

	repaint();
}

bool VlcVideoPlayer::isVideoOpen() const
{
	return videoLoaded;
}

File VlcVideoPlayer::getCurrentVideoFile() const
{
	return currentFile;
}

double VlcVideoPlayer::getVideoDuration() const
{
	return durationMs / 1000.0;
}

double VlcVideoPlayer::getPlayPosition() const
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();
	if (player == nullptr || !vlc.isReady()) return 0;

	long long timeMs = vlc.player_get_time(player);
	if (timeMs < 0) return 0;

	return timeMs / 1000.0;
}

void VlcVideoPlayer::setPlayPosition(double seconds)
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();
	if (player == nullptr || !vlc.isReady()) return;

	vlc.player_set_time(player, (long long) (seconds * 1000.0));
}

void VlcVideoPlayer::setPlaySpeed(double rate)
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();
	if (player == nullptr || !vlc.isReady()) return;

	vlc.player_set_rate(player, (float) jlimit(0.02, 100.0, rate));
}

void VlcVideoPlayer::setAudioVolume(float volume)
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();
	if (player == nullptr || !vlc.isReady()) return;

	vlc.audio_set_volume(player, roundToInt(jlimit(0.0f, 1.0f, volume) * 100.0f));
}

void VlcVideoPlayer::play()
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();
	if (player == nullptr || !vlc.isReady()) return;

	if (vlc.player_get_state(player) == vlcStateEnded)
		vlc.player_set_pause(player, 0);

	vlc.player_play(player);
}

void VlcVideoPlayer::pause()
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();
	if (player == nullptr || !vlc.isReady()) return;

	if (vlc.player_get_state(player) == vlcStatePlaying)
		vlc.player_set_pause(player, 1);
}

void VlcVideoPlayer::stop()
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();
	if (player == nullptr || !vlc.isReady()) return;

	if (vlc.player_get_state(player) == vlcStatePlaying)
		vlc.player_set_pause(player, 1);

	vlc.player_set_time(player, 0);
}

bool VlcVideoPlayer::isPlaying() const
{
	LibVCLLibrary& vlc = LibVCLLibrary::get();
	if (player == nullptr || !vlc.isReady()) return false;

	return vlc.player_get_state(player) == vlcStatePlaying;
}

juce::Image VlcVideoPlayer::getCurrentFrame() const
{
	return frameStore != nullptr ? frameStore->getFront() : juce::Image();
}

void VlcVideoPlayer::setRenderTransform(float opacity, float scaleX, float scaleY, float xPercent, float yPercent)
{
	if (renderState.opacity == opacity
		&& renderState.scaleX == scaleX
		&& renderState.scaleY == scaleY
		&& renderState.xPct == xPercent
		&& renderState.yPct == yPercent) return;

	renderState.opacity = opacity;
	renderState.scaleX = scaleX;
	renderState.scaleY = scaleY;
	renderState.xPct = xPercent;
	renderState.yPct = yPercent;

	repaint();
}

void VlcVideoPlayer::drawFrameWithTransform(Graphics& g, const Image& img, Rectangle<int> targetArea, float opacity, float scaleX, float scaleY, float xPercent, float yPercent)
{
	if (!img.isValid() || targetArea.isEmpty()) return;

	RectanglePlacement placement(RectanglePlacement::centred);
	const Rectangle<int> fit = placement.appliedTo(Rectangle<int>(img.getWidth(), img.getHeight()), targetArea);

	const float w = fit.getWidth() * scaleX;
	const float h = fit.getHeight() * scaleY;

	Rectangle<float> dest(fit.getCentreX() - w * 0.5f, fit.getCentreY() - h * 0.5f, w, h);
	dest.translate(targetArea.getWidth() * xPercent / 100.0f, targetArea.getHeight() * yPercent / 100.0f);

	if (dest.isEmpty()) return;

	g.setOpacity(jlimit(0.0f, 1.0f, opacity));
	g.drawImage(img, dest, RectanglePlacement(RectanglePlacement::stretchToFit));
	g.setOpacity(1.0f);
}

void VlcVideoPlayer::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF111111));

	Image img = frameStore != nullptr ? frameStore->getFront() : Image();

	if (img.isValid())
	{
		drawFrameWithTransform(g, img, getLocalBounds(), renderState.opacity, renderState.scaleX, renderState.scaleY, renderState.xPct, renderState.yPct);
	}
}

void VlcVideoPlayer::handlePlaybackStoppedEvent()
{
	if (onPlaybackStopped != nullptr)
		onPlaybackStopped();
}

void VlcVideoPlayer::handleAsyncUpdate()
{
	if (hasNewFrame.exchange(false))
	{
		if (onFrameDecoded != nullptr)
			onFrameDecoded(getCurrentFrame());
		repaint();
	}

	if (needsStoppedEvent.exchange(false))
		handlePlaybackStoppedEvent();
}
