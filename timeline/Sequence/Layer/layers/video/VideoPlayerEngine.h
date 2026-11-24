/*
  ==============================================================================

	VideoPlayerEngine.h
	Abstract base interface for video playback engines (MPV, VLC, etc.)

	The engine is message-thread oriented : all methods (except the GL callbacks
	documented below) are meant to be called from the message thread. Long-lived
	asynchronous operations report their completion through the Listener
	callbacks, which the engine marshals onto the message thread for the user.

	GL callbacks (setupGL / renderGL / clearGL) are invoked from the dedicated
	OpenGL thread while its context is current, see the Chataigne GL context
	holder.

  ==============================================================================
*/

#pragma once

class VideoPlayerEngine
{
public:
	class Listener
	{
	public:
		virtual ~Listener() {}

		// A file finished loading : getDuration() / getVideoWidth() / etc are valid now.
		virtual void playerFileLoaded(VideoPlayerEngine*) {}
		// Playback position changed (informational).
		virtual void playerTimeChanged(double time) {}
		// A new video frame is available and should be re-rendered.
		virtual void playerFrameUpdate() {}
		// Playback reached the end of the file (or was stopped).
		virtual void playerFileEnd(VideoPlayerEngine*) {}
	};

	VideoPlayerEngine() {}
	virtual ~VideoPlayerEngine() {}

	// Lifecycle. load() returns true when the file was accepted for loading ;
	// completion is reported asynchronously via Listener::playerFileLoaded().
	// When the engine has no GL context yet, it stores the path and starts
	// loading as soon as setupGL() succeeds later.
	virtual bool load(const juce::String& filePath) = 0;
	virtual void unload() = 0;

	// Loading state
	virtual bool isFileLoaded() const = 0;
	virtual juce::String getFilePath() const = 0;
	virtual bool isGLInit() const { return false; }

	// Playback control
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;
	virtual bool isPlaying() const = 0;

	// Seeking & position
	virtual void setPosition(double pos) = 0;
	virtual double getPosition() const = 0;
	virtual double getDuration() const = 0;

	// Playback parameters
	virtual void setPlaySpeed(float speed) = 0;
	virtual float getPlaySpeed() const = 0;
	virtual void setVolume(float volume) = 0;
	virtual float getVolume() const = 0;
	virtual void setLoop(bool loop) = 0;
	virtual bool getLoop() const = 0;

	// Video properties
	virtual int getVideoWidth() const = 0;
	virtual int getVideoHeight() const = 0;
	virtual int getNumChannels() const = 0;

	// GL rendering. All of these run with the engine's own OpenGL context current.
	virtual void setupGL() = 0;
	virtual void renderGL(juce::OpenGLFrameBuffer& target) = 0;
	virtual void clearGL() {}

	// Audio that is mixed into the host audio graph, if the engine provides any.
	virtual juce::AudioProcessor* getAudioProcessor() = 0;

	// Event polling : drains engine events and fires the Listener callbacks on
	// the message thread. Called at a fixed rate by the host.
	virtual void pullEvents() {}

	// Listener management
	void addListener(Listener* listener) { listeners.add(listener); }
	void removeListener(Listener* listener) { listeners.remove(listener); }

	void notifyFileLoaded() { listeners.call(&Listener::playerFileLoaded, this); }
	void notifyTimeChanged(double time) { listeners.call(&Listener::playerTimeChanged, time); }
	void notifyFrameUpdate() { listeners.call(&Listener::playerFrameUpdate); }
	void notifyFileEnd() { listeners.call(&Listener::playerFileEnd, this); }

protected:
	juce::ListenerList<Listener> listeners;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoPlayerEngine)
};

// ==============================================================================
// No-op engine : used by the base VideoLayer when no concrete engine is provided
// (the factory is overridden by the host app, e.g. ChataigneVideoLayer). Every
// call is a harmless default so the layer can always be constructed and edited.
// ==============================================================================

class NullVideoPlayer :
	public VideoPlayerEngine
{
public:
	NullVideoPlayer() {}
	~NullVideoPlayer() override {}

	bool load(const juce::String& filePath) override
	{
		currentFilePath = filePath;
		return true;
	}

	void unload() override {}

	bool isFileLoaded() const override { return false; }
	juce::String getFilePath() const override { return currentFilePath; }
	bool isGLInit() const override { return false; }

	void play() override {}
	void pause() override {}
	void stop() override {}
	bool isPlaying() const override { return false; }

	void setPosition(double) override {}
	double getPosition() const override { return 0; }
	double getDuration() const override { return 0; }

	void setPlaySpeed(float) override {}
	float getPlaySpeed() const override { return 1.0f; }
	void setVolume(float) override {}
	float getVolume() const override { return 1.0f; }
	void setLoop(bool) override {}
	bool getLoop() const override { return false; }

	int getVideoWidth() const override { return 0; }
	int getVideoHeight() const override { return 0; }
	int getNumChannels() const override { return 0; }

	void setupGL() override {}
	void renderGL(juce::OpenGLFrameBuffer&) override {}
	void clearGL() override {}

	juce::AudioProcessor* getAudioProcessor() override { return nullptr; }

private:
	juce::String currentFilePath;
};
