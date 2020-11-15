/*
  ==============================================================================

    AudioLayerClip.h
    Created: 8 Feb 2017 12:20:02pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class AudioLayerClip :
	public LayerBlock,
	public Thread //async loading
{
public:
	AudioLayerClip();
	virtual ~AudioLayerClip();

	AudioFormatManager formatManager;
	//AudioSampleBuffer buffer;
	std::unique_ptr<AudioFormatReaderSource> readerSource;
	AudioTransportSource transportSource;
	ResamplingAudioSource resamplingAudioSource;
	ChannelRemappingAudioSource channelRemapAudioSource;

	FileParameter * filePath;

	FloatParameter * clipLength;
	FloatParameter* clipStartOffset;
	
	FloatParameter* fadeIn;
	FloatParameter* fadeOut;

	FloatParameter * stretchFactor;
	Trigger* resetStretch;

	FloatParameter * volume;

	double clipDuration;
	double sampleRate;
	int clipSamplePos;

	bool isCurrent;
	bool isLoading;

	void setIsCurrent(bool value);
	
	bool isInRange(float time);

	void updateAudioSourceFile();
	void onContainerTriggerTriggered(Trigger* t) override;
	void onContainerParameterChangedInternal(Parameter *) override;

	void setCoreLength(float value, bool stretch, bool stickToCoreEnd = false) override;
	void setStartTime(float value, bool stretch, bool stickToCoreEnd = false) override;
	
	void setPlaySpeed(float value);

	void run() override;

	String getTypeString() const override { return "AudioClip"; }

	class ClipListener
	{
	public:
		virtual ~ClipListener() {}
		virtual void clipSourceLoaded(AudioLayerClip*) {}
		virtual void clipIsCurrentChanged(AudioLayerClip *) {}
	};

	ListenerList<ClipListener> clipListeners;
	void addClipListener(ClipListener* newListener) { clipListeners.add(newListener); }
	void removeClipListener(ClipListener* listener) { clipListeners.remove(listener); }

	class ClipEvent {
	public:
		enum Type { CLIP_IS_CURRENT_CHANGED, SOURCE_LOAD_START, SOURCE_LOAD_END };
		ClipEvent(Type type, AudioLayerClip * i) : type(type), clip(i) {}
		Type type;
		AudioLayerClip *clip;
	};

	QueuedNotifier<ClipEvent> audioClipAsyncNotifier;
	typedef QueuedNotifier<ClipEvent>::Listener AsyncListener;

	void addAsyncClipListener(AsyncListener* newListener) { audioClipAsyncNotifier.addListener(newListener); }
	void addAsyncCoalescedClipListener(AsyncListener* newListener) { audioClipAsyncNotifier.addAsyncCoalescedListener(newListener); }
	void removeAsyncClipListener(AsyncListener* listener) { audioClipAsyncNotifier.removeListener(listener); }


private:
	WeakReference<AudioLayerClip>::Master masterReference;
	friend class WeakReference<AudioLayerClip>;

};


