/*
  ==============================================================================

    VideoLayerClip.h
    Created: 26 Sep 2026

  ==============================================================================
*/

#pragma once

static juce::String timeValueToString(double t)
{
	int totalMs = juce::jmax(0, (int)juce::roundToInt(t * 1000.0));
	int ms = totalMs % 1000;
	int totalSeconds = totalMs / 1000;
	int s = totalSeconds % 60;
	int m = totalSeconds / 60;
	return juce::String(m) + ":" + juce::String(s).paddedLeft('0', 2) + "." + juce::String(ms).paddedLeft('0', 3);
}

class VideoLayerClip :
	public LayerBlock
{
public:
	VideoLayerClip();
	virtual ~VideoLayerClip();

	FileParameter* filePath;

	FloatParameter* clipLength;
	FloatParameter* clipStartOffset;

	FloatParameter* volume;

	FloatParameter* opacity;

	ControllableContainer* transform;
	FloatParameter* width;
	FloatParameter* height;
	FloatParameter* size;
	FloatParameter* x;
	FloatParameter* y;

	enum class BlendMode
	{
		Normal,
		Add,
		Multiply,
		Screen,
		Lighten,
		Darken,
		Overlay,
		Difference,
		Exclusion
	};

	EnumParameter* blendMode;

	double clipDuration;

	float getRenderOpacity() const { return opacity->floatValue(); }
	float getRenderScaleX() const { return (size->floatValue() / 100.0f) * (width->floatValue() / 100.0f); }
	float getRenderScaleY() const { return (size->floatValue() / 100.0f) * (height->floatValue() / 100.0f); }
	float getRenderXPercent() const { return x->floatValue(); }
	float getRenderYPercent() const { return y->floatValue(); }
	BlendMode getBlendMode() const { return (BlendMode)(int) blendMode->getValueData(); }

	void onContainerParameterChangedInternal(Parameter* p) override;
	void setCoreLength(float value, bool stretch, bool stickToCoreEnd = false) override;
	void setStartTime(float value, bool stretch, bool stickToCoreEnd = false) override;

	DECLARE_TYPE("VideoClip");

	class ClipListener
	{
	public:
		virtual ~ClipListener() {}
		virtual void clipSourceLoaded(VideoLayerClip*) {}
		virtual void clipParamChanged(VideoLayerClip*) {}
	};

	ListenerList<ClipListener> clipListeners;
	void addClipListener(ClipListener* newListener) { clipListeners.add(newListener); }
	void removeClipListener(ClipListener* listener) { clipListeners.remove(listener); }

private:
	WeakReference<VideoLayerClip>::Master masterReference;
	friend class WeakReference<VideoLayerClip>;

};