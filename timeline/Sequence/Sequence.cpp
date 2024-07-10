/*
  ==============================================================================

	Sequence.cpp
	Created: 28 Oct 2016 8:13:19pm
	Author:  bkupe

  ==============================================================================
*/

#include "JuceHeader.h"
#include "Sequence.h"

Sequence::Sequence() :
	BaseItem("Sequence", true),
	Thread("Sequence"),
	currentManager(nullptr),
	hiResAudioTime(0),
	sampleRate(44100),
	isSeeking(false),
	//timeAtSetTime(0),
	millisAtSetTime(0),
	prevMillis(0),
	targetTime(0),
	isBeingEdited(false),
	sequenceNotifier(10)
{
	isPlaying = addBoolParameter("Is Playing", "Is the sequence playing ?", false);
	isPlaying->setControllableFeedbackOnly(true);
	//isPlaying->isSavable = false;
	//isPlaying->hideInEditor = true;

	playTrigger = addTrigger("Play", "Play the sequence");
	stopTrigger = addTrigger("Stop", "Stops the sequence and set the current time at 0.");
	finishTrigger = addTrigger("Finish", "When the sequence reached naturally its end, and there is no loop");
	finishTrigger->hideInEditor = true;
	pauseTrigger = addTrigger("Pause", "Pause the sequence and keep the current time as is.");
	togglePlayTrigger = addTrigger("TogglePlay", "Toggle between play/pause or play/stop depending on sequence settings");

	float initTotalTime = 30; //default to 30 seconds, may be in general preferences later

	startAtLoad = addBoolParameter("Play at Load", "If selected, the sequence will start playing just after loading the file", false);

	includeCurrentTimeInSave = addBoolParameter("Save Current Time", "If checked, this will save and load the current time in the file. Otherwise the current time will be reset to 0 on file load.", false);

	currentTime = addFloatParameter("Current Time", "Current position in time of this sequence", 0, 0, initTotalTime);
	currentTime->defaultUI = FloatParameter::TIME;
	currentTime->isSavable = false;


	totalTime = addFloatParameter("Total Time", "Total time of this sequence, in seconds", initTotalTime, minSequenceTime);
	totalTime->defaultUI = FloatParameter::TIME;

	playSpeed = addFloatParameter("Play Speed", "Playing speed factor, 1 is normal speed, 2 is double speed and 0.5 is half speed", 1);
	prevSpeed = playSpeed->floatValue();

	fps = addIntParameter("FPS", "Frame Per Second.\nDefines the number of times per seconds the sequence is evaluated, the higher the value is, the more previse the calculation will be.\n \
									This setting also sets how many messages per seconds are sent from layer with automations.", 50, 1, 500);
	loopParam = addBoolParameter("Loop", "Whether the sequence plays again from the start when reached the end while playing", false);

	bpmPreview = addFloatParameter("BPM Preview", "BPM for previewing in the timeline.", 120, 1, 999, false);
	bpmPreview->canBeDisabledByUser = true;
	beatsPerBar = addIntParameter("Beats Per Bar", "Number of beats in a bar. Only for preview in UI", 4, 1, 32, false);

	autoSnap = addBoolParameter("Auto Snap", "If checked, this will automatically snap when moving items", false);

	evaluateOnSeek = addEnumParameter("Evaluate on Seek", "This decides when it should evaluate data (like triggering time triggers) when seeking manually.");
	evaluateOnSeek->addOption("When Playing Only", ONLY_PLAYING)->addOption("When Not Playing", ONLY_NOT_PLAYING)->addOption("Always", ALWAYS)->addOption("Never", NEVER);

	currentTime->unitSteps = fps->intValue();
	totalTime->unitSteps = fps->intValue();

	prevCue = addTrigger("Prev Cue", "Jump to previous cue, if previous cue is less than 1 sec before, jump to the one before that.");
	nextCue = addTrigger("Next Cue", "Jump to the next cue");

	viewStartTime = addFloatParameter("View start time", "Start time of the view", 0, 0, initTotalTime - .01f);
	viewEndTime = addFloatParameter("View end time", "End time of the view", initTotalTime, 0.01f, initTotalTime);
	minViewTime = addFloatParameter("Minimum view range", "Minimum range of the view when zooming", 1.f, 0.1f);
	viewStartTime->defaultUI = FloatParameter::TIME;
	viewEndTime->defaultUI = FloatParameter::TIME;
	minViewTime->defaultUI = FloatParameter::TIME;

	viewFollowTime = addBoolParameter("View follow time", "If checked, this will automatically follow the current time so the cursor is at the center of the timeline.", false);

	setHasCustomColor(true);
	itemColor->setDefaultValue(BG_COLOR.brighter(.1f));

	layerManager.reset(new SequenceLayerManager(this));
	addChildControllableContainer(layerManager.get());

	cueManager.reset(new TimeCueManager());
	addChildControllableContainer(cueManager.get());
	cueManager->hideInEditor = true;

	listUISize->setValue(30);
	listUISize->isSavable = false;
}

Sequence::~Sequence()
{
	clearItem();
}

void Sequence::clearItem()
{
	BaseItem::clearItem();

	setAudioDeviceManager(nullptr);

	stopThread(500);

	//if(!Engine::mainEngine->isClearing) stopTrigger->trigger();
	if (Engine::mainEngine != nullptr) Engine::mainEngine->removeEngineListener(this);
}

void Sequence::setCurrentTime(float time, bool forceOverPlaying, bool seekMode)
{
	time = jlimit<float>(0, totalTime->floatValue(), time);

	if (isPlaying->boolValue() && !forceOverPlaying) return;

	GenericScopedLock lock(sequenceTimeLock);

	isSeeking = seekMode;

	millisAtSetTime = Time::getMillisecondCounterHiRes();
	//timeAtSetTime = time;
	if (seekMode || forceOverPlaying) targetTime = time;

	if (timeIsDrivenByAudio())
	{
		hiResAudioTime = time;
		if (!isPlaying->boolValue() || isSeeking || forceOverPlaying) currentTime->setValue(time);
	}
	else
	{
		currentTime->setValue(time);
	}

	isSeeking = false;
}

void Sequence::handleCueAction(TimeCue* cue, TimeCue* originCue)
{
	if (cue == nullptr) return;
	if (!cue->isCurrentlyActive()) return;

	if (originCue == nullptr) originCue = cue;

	TimeCue::CueAction a = cue->cueAction->getValueDataAsEnum<TimeCue::CueAction>();
	switch (a)
	{
	case TimeCue::PAUSE:
	{
		pauseTrigger->trigger();
		prevTime = currentTime->floatValue();
		setCurrentTime(cue->time->floatValue());
		return;
	default:
		break;
	}
	break;

	case TimeCue::LOOP_JUMP:
	{
		if (TimeCue* tc = dynamic_cast<TimeCue*>(cue->loopCue->targetContainer.get()))
		{
			if (tc != cue && tc != originCue)
			{
				setCurrentTime(tc->time->floatValue(), true, true);
				TimeCue::CueAction a = cue->cueAction->getValueDataAsEnum<TimeCue::CueAction>();
				if (a != TimeCue::NOTHING) handleCueAction(tc, originCue);
			}
		}
	}
	break;
	}
}

int Sequence::getFrameForTime(float time, bool forceDirection, bool forcePrev)
{
	float f = time * fps->floatValue() / (playSpeed->floatValue() != 0 ? playSpeed->floatValue() : 1.0f);
	return forceDirection ? (forcePrev ? floorf(f) : ceilf(f)) : round(f);
}

double Sequence::getTimeForFrame(float frame)
{
	return frame * 1.0 / (fps->floatValue() / (playSpeed->floatValue() != 0 ? playSpeed->floatValue() : 1.0f));
}

double Sequence::getNextFrameTimeForTime(float time)
{
	return getTimeForFrame(getFrameForTime(time, true, false));
}

double Sequence::getPrevFrameTimeForTime(float time)
{
	return getTimeForFrame(getFrameForTime(time, true, true));
}

void Sequence::setBeingEdited(bool value)
{
	if (isBeingEdited == value) return;
	isBeingEdited = value;
	sequenceListeners.call(&SequenceListener::sequenceEditingStateChanged, this);
	sequenceNotifier.addMessage(new SequenceEvent(SequenceEvent::EDITING_STATE_CHANGED, this));
}

void Sequence::selectAllItemsBetween(float start, float end)
{
	Array<Inspectable*> selection;
	for (auto& l : layerManager->items) selection.addArray(l->selectAllItemsBetween(start, end));
	InspectableSelectionManager::mainSelectionManager->selectInspectables(selection);
}

void Sequence::removeAllItemsBetween(float start, float end)
{
	Array<UndoableAction*> actions;
	for (auto& l : layerManager->items)  actions.addArray(l->getRemoveAllItemsBetween(start, end));
	UndoMaster::getInstance()->performActions("Remove items between timespan", actions);
}

void Sequence::removeTimespan(float start, float end)
{
	Array<UndoableAction*> actions;
	for (auto& l : layerManager->items)  actions.addArray(l->getRemoveTimespan(start, end));
	actions.addArray(cueManager->getRemoveTimespan(start, end));
	actions.add(totalTime->setUndoableValue(totalTime->floatValue(), totalTime->floatValue() - (end - start), true));
	UndoMaster::getInstance()->performActions("Remove timespan", actions);
}

void Sequence::insertTimespan(float start, float length)
{
	Array<UndoableAction*> actions;
	actions.add(totalTime->setUndoableValue(totalTime->floatValue(), totalTime->floatValue() + length, true));
	for (auto& l : layerManager->items)  actions.addArray(l->getInsertTimespan(start, length));
	actions.addArray(cueManager->getInsertTimespan(start, length));
	UndoMaster::getInstance()->performActions("Insert timespan", actions);
}

void Sequence::getSnapTimes(Array<float>* arrayToFill, float start, float end, const Array<float>& excludeValues)
{
	if (end == -1) end = totalTime->floatValue();

	for (auto& i : layerManager->items) i->getSnapTimes(arrayToFill);

	cueManager->getSnapTimes(arrayToFill);
	arrayToFill->addIfNotAlreadyThere(currentTime->floatValue());

	if (bpmPreview->enabled)
	{
		float step = 60.0 / bpmPreview->floatValue();
		for (float i = start; i < end; i += step) arrayToFill->addIfNotAlreadyThere(i);
	}

	arrayToFill->removeValuesIn(excludeValues);
	if (start > 0 || end < totalTime->floatValue()) arrayToFill->removeIf([start, end](float v) {return v >= start && v <= end; });
}

float Sequence::getClosestSnapTimeFor(Array<float> snapTimes, float time)
{
	float result = time;
	float diff = INT32_MAX;
	for (auto& t : snapTimes)
	{
		float d = fabsf(time - t);
		if (d < diff)
		{
			diff = d;
			result = t;
		}
	}
	return result;
}

bool Sequence::paste()
{
	Array<SequenceLayer*> p = layerManager->addItemsFromClipboard(false);
	if (p.isEmpty()) return BaseItem::paste();
	return true;
}

void Sequence::setAudioDeviceManager(AudioDeviceManager* manager)
{
	if (currentManager == manager) return;

	if (currentManager != nullptr) currentManager->removeAudioCallback(this);

	currentManager = manager;

	if (currentManager != nullptr) currentManager->addAudioCallback(this);


	//resync values between audio/non-audio driving variables
	hiResAudioTime = (double)currentTime->floatValue();

	sequenceListeners.call(&SequenceListener::sequenceMasterAudioModuleChanged, this);
}

void Sequence::updateSampleRate()
{
	AudioDeviceManager::AudioDeviceSetup s;
	if (currentManager != nullptr) currentManager->getAudioDeviceSetup(s);
	if (s.sampleRate != 0) sampleRate = s.sampleRate;
}

bool Sequence::timeIsDrivenByAudio()
{
	return currentManager != nullptr;
}

var Sequence::getJSONData()
{
	var data = BaseItem::getJSONData();
	var layerData = layerManager->getJSONData();
	if (!layerData.isVoid()) data.getDynamicObject()->setProperty(layerManager->shortName, layerData);
	var cueData = cueManager->getJSONData();
	if (!cueData.isVoid()) data.getDynamicObject()->setProperty(cueManager->shortName, cueData);
	if (isBeingEdited) data.getDynamicObject()->setProperty("editing", true);
	return data;
}

void Sequence::loadJSONDataInternal(var data)
{
	BaseItem::loadJSONDataInternal(data);
	layerManager->loadJSONData(data.getProperty(layerManager->shortName, var()));
	cueManager->loadJSONData(data.getProperty(cueManager->shortName, var()));
	isBeingEdited = data.getProperty("editing", false);

	if (Engine::mainEngine->isLoadingFile)
	{
		Engine::mainEngine->addEngineListener(this);
	}

}

void Sequence::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == enabled)
	{

	}
	else if (p == currentTime)
	{
		if (isPlaying->boolValue() && !isSeeking)
		{
			float minTime = jmin<float>(prevTime, currentTime->floatValue());
			float maxTime = jmax<float>(prevTime, currentTime->floatValue());
			bool playingForward = playSpeed->floatValue() > 0;
			Array<TimeCue*> cues = cueManager->getCuesInTimespan(minTime, maxTime, !playingForward, playingForward);
			if (cues.size() > 0) handleCueAction(cues[0]);
		}

		if ((!isPlaying->boolValue() || isSeeking) && timeIsDrivenByAudio()) hiResAudioTime = currentTime->floatValue();
		else if (getCurrentThreadId() != getThreadId())
		{
			millisAtSetTime = Time::getMillisecondCounterHiRes();
			//timeAtSetTime = timeIsDrivenByAudio() ? hiResAudioTime : currentTime->floatValue();
		}

		EvaluateMode e = evaluateOnSeek->getValueDataAsEnum<EvaluateMode>();
		bool shouldEvaluate = e == ALWAYS || (e == ONLY_PLAYING && isPlaying->boolValue()) || (e == ONLY_NOT_PLAYING && !isPlaying->boolValue());
		sequenceListeners.call(&SequenceListener::sequenceCurrentTimeChanged, this, (float)prevTime, shouldEvaluate);
		prevTime = currentTime->floatValue();
	}
	else if (p == totalTime)
	{
		//float minViewTime = minSequenceTime;// jmax(minSequenceTime, totalTime->floatValue() / 100.f); //small hack to avoid UI hang when zooming too much

		currentTime->setRange(0, totalTime->floatValue());
		viewStartTime->setRange(0, totalTime->floatValue() - minViewTime->floatValue());
		viewEndTime->setRange(viewStartTime->floatValue() + minViewTime->floatValue(), totalTime->floatValue());
		sequenceListeners.call(&SequenceListener::sequenceTotalTimeChanged, this);
	}
	else if (p == isPlaying)
	{
		signalThreadShouldExit();
		notify();
		if (getCurrentThreadId() != getThreadId())
		{
			waitForThreadToExit(300);
		}

		if (isPlaying->boolValue())
		{
			if (currentTime->floatValue() >= totalTime->floatValue())
			{
				hiResAudioTime = 0;
				setCurrentTime(0, true, true); //if reached the end when hit play, go to 0
			}

			prevTime = currentTime->floatValue();
			if (!isThreadRunning()) startThread();
		}

		sequenceListeners.call(&SequenceListener::sequencePlayStateChanged, this);
		sequenceNotifier.addMessage(new SequenceEvent(SequenceEvent::PLAY_STATE_CHANGED, this));
	}
	else if (p == playSpeed)
	{
		if ((prevSpeed < 0 && playSpeed->floatValue() > 0) || (prevSpeed > 0 && playSpeed->floatValue() < 0))
		{
			sequenceListeners.call(&SequenceListener::sequencePlayDirectionChanged, this);
		}

		prevSpeed = playSpeed->floatValue();
		sequenceListeners.call(&SequenceListener::sequencePlaySpeedChanged, this);
	}
	else if (p == viewStartTime)
	{
		//float minViewTime = jmax(minSequenceTime, totalTime->floatValue() / 100.f); //small hack to avoid UI hang when zooming too much
		viewEndTime->setRange(viewStartTime->floatValue() + minViewTime->floatValue(), totalTime->floatValue()); //Should be a range value
	}
	else if (p == includeCurrentTimeInSave)
	{
		currentTime->isSavable = includeCurrentTimeInSave->boolValue();
	}

	if (p == fps || p == playSpeed)
	{
		float steps = fps->floatValue() / (playSpeed->floatValue() != 0 ? playSpeed->floatValue() : 1.0f);
		currentTime->unitSteps = steps;
		totalTime->unitSteps = steps;

		//right now this makes crash because of cross listener call during threads
		totalTime->setValue(totalTime->floatValue()); //force update
		setCurrentTime(currentTime->floatValue()); //force update
	}
}

void Sequence::onContainerTriggerTriggered(Trigger* t)
{
	if (t == playTrigger)
	{
		isPlaying->setValue(true);
	}
	else if (t == stopTrigger)
	{
		isPlaying->setValue(false);
		setCurrentTime(0, true, true);
	}
	else if (t == pauseTrigger)
	{
		isPlaying->setValue(false);
	}
	else if (t == finishTrigger)
	{
		isPlaying->setValue(false);
		sequenceListeners.call(&SequenceListener::sequenceFinished, this);
	}
	else if (t == togglePlayTrigger)
	{
		if (isPlaying->boolValue()) pauseTrigger->trigger();
		else playTrigger->trigger();

	}
	else if (t == prevCue)
	{
		setCurrentTime(cueManager->getPrevCueForTime(currentTime->floatValue(), 1), true, true);
	}
	else if (t == nextCue)
	{
		setCurrentTime(cueManager->getNextCueForTime(currentTime->floatValue()), true, true);
	}
}

void Sequence::onControllableStateChanged(Controllable* c)
{
	BaseItem::onControllableStateChanged(c);
	if (c == bpmPreview)
	{
		beatsPerBar->setEnabled(bpmPreview->enabled);
	}
}

void Sequence::parameterControlModeChanged(Parameter* p)
{
	BaseItem::parameterControlModeChanged(p);
	if (p == currentTime)
	{
		includeCurrentTimeInSave->setValue(currentTime->controlMode != Parameter::MANUAL);
		includeCurrentTimeInSave->setEnabled(currentTime->controlMode == Parameter::MANUAL);
	}
}

bool Sequence::handleRemoteControlData(Controllable* c, const juce::OSCMessage& m, const juce::String& cliendId)
{
	if (c == currentTime)
	{
		if (m.size() >= 1) setCurrentTime(OSCHelpers::getFloatArg(m[0]), true, true);
		return true;
	}
	return false;
}

String Sequence::getPanelName() const
{
	return niceName;
}

void Sequence::run()
{
	millisAtSetTime = Time::getMillisecondCounterHiRes();
	//timeAtSetTime = timeIsDrivenByAudio() ? hiResAudioTime : currentTime->floatValue();
	prevMillis = Time::getMillisecondCounterHiRes();

	followViewRange = viewEndTime->floatValue() - viewStartTime->floatValue();

	targetTime = currentTime->floatValue();

	while (!threadShouldExit())
	{

		double millis = Time::getMillisecondCounterHiRes();
		//double millisSinceSetTime = millis - millisAtSetTime;
		double delta = millis - prevMillis;

		targetTime += (delta / 1000.0) * playSpeed->floatValue();

		prevMillis = millis;

		//double absoluteTargetTime = timeAtSetTime + (millisSinceSetTime / 1000.0) * playSpeed->floatValue();

		if (timeIsDrivenByAudio())
		{
			//DBG("Diff (ms): " << abs(hiResAudioTime - currentTime->floatValue()));
			//targetTime = hiResAudioTime;
		}

		//DBG(deltaMillis << " : " << (targetTime - currentTime->floatValue()));

		if (!isSeeking) setCurrentTime(targetTime);

		if (viewFollowTime->boolValue())
		{
			float targetStart = jmax(currentTime->floatValue() - followViewRange / 2, 0.f);
			float targetEnd = targetStart + followViewRange;
			if (targetEnd > totalTime->floatValue())
			{
				targetEnd = totalTime->floatValue();
				targetStart = targetEnd - followViewRange;
			}

			viewStartTime->setValue(viewStartTime->getLerpValueTo(targetStart, .3f));
			viewEndTime->setValue(viewEndTime->getLerpValueTo(targetEnd, .3f));
		}

		if (targetTime >= totalTime->floatValue())
		{
			if (loopParam->boolValue())
			{
				float offset = targetTime - totalTime->floatValue();
				sequenceListeners.call(&SequenceListener::sequenceLooped, this);
				//setCurrentTime(0); //to change in trigger layer to avoid doing that
				prevTime = 0;
				setCurrentTime(offset, true, true);
			}
			else finishTrigger->trigger();
		}

		double millisPerCycle = 1000.0 / fps->floatValue();
		double millisAfterProcess = Time::getMillisecondCounterHiRes();
		double relAbsMillis = millisAfterProcess - millisAtSetTime;
		double millisToWait = ceil(millisPerCycle - fmod(relAbsMillis, millisPerCycle));

		if (millisToWait >= 0) wait(millisToWait);
	}
}

void Sequence::endLoadFile()
{
	Engine::mainEngine->removeEngineListener(this);
	if (isBeingEdited) selectThis();

	handleStartAtLoad();
}

void Sequence::handleStartAtLoad()
{
	if (startAtLoad->boolValue())
	{
		playTrigger->trigger();
	}
}


void Sequence::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
	int numInputChannels,
	float* const* outputChannelData,
	int numOutputChannels,
	int numSamples,
	const AudioIODeviceCallbackContext& context)
{
	for (int i = 0; i < numOutputChannels; ++i) FloatVectorOperations::clear(outputChannelData[i], numSamples);

	if (isPlaying->boolValue())
	{
		hiResAudioTime += (numSamples / sampleRate) * playSpeed->floatValue();
	}
}
void Sequence::audioDeviceAboutToStart(AudioIODevice*)
{
	updateSampleRate();
}

void Sequence::audioDeviceStopped()
{
}
