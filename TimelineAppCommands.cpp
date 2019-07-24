#include "TimelineAppCommands.h"


ControllableContainer* TimelineAppCommands::timelineSettingsContainer = nullptr;
FloatParameter* TimelineAppCommands::stepTime = nullptr;


namespace TimelineCommandIDs
{
	const int playPauseSequenceEditor = 0x80000;
	const int prevCue = 0x80001;
	const int nextCue = 0x80002;
	const int prevTimeStep = 0x80003;
	const int nextTimeStep = 0x80004;
	const int goToStart = 0x80005;
	const int goToEnd = 0x80006;

}

void TimelineAppCommands::init()
{
	timelineSettingsContainer = new ControllableContainer("Timeline");
	stepTime = timelineSettingsContainer->addFloatParameter("Step time", "Time to do the stepping using keyboard shortcuts", 1, 0);
	stepTime->defaultUI = FloatParameter::TIME;
	GlobalSettings::getInstance()->addChildControllableContainer(timelineSettingsContainer, true, GlobalSettings::getInstance()->controllableContainers.size()-1);
}

void TimelineAppCommands::getCommandInfo(CommandID commandID, ApplicationCommandInfo& result)
{


	switch (commandID)
	{

	case TimelineCommandIDs::playPauseSequenceEditor:
		result.setInfo("Play/Pause Sequence", "", "Timeline", 0);
		result.addDefaultKeypress(KeyPress::spaceKey, ModifierKeys::noModifiers);
		break;

	case TimelineCommandIDs::prevCue:
		result.setInfo("Previous cue", "", "Timeline", 0);
		result.addDefaultKeypress(KeyPress::pageUpKey, ModifierKeys::shiftModifier);
		break;

	case TimelineCommandIDs::nextCue:
		result.setInfo("Next cue", "", "Timeline", 0);
		result.addDefaultKeypress(KeyPress::pageDownKey, ModifierKeys::shiftModifier);
		break;

	case TimelineCommandIDs::prevTimeStep:
		result.setInfo("Previous time step", "", "Timeline", 0);
		result.addDefaultKeypress(KeyPress::pageUpKey, ModifierKeys::noModifiers);
		break;

	case TimelineCommandIDs::nextTimeStep:
		result.setInfo("Next time step", "", "Timeline", 0);
		result.addDefaultKeypress(KeyPress::pageDownKey, ModifierKeys::noModifiers);
		break;

	case TimelineCommandIDs::goToStart:
		result.setInfo("Go to start", "", "Timeline", 0);
		result.addDefaultKeypress(KeyPress::homeKey, ModifierKeys::noModifiers);
		break;

	case TimelineCommandIDs::goToEnd:
		result.setInfo("Go toend", "", "Timeline", 0);
		result.addDefaultKeypress(KeyPress::endKey, ModifierKeys::noModifiers);
		break;
	}
}


void TimelineAppCommands::getAllCommands(Array<CommandID>& commands) {

	const CommandID ids[] = {

		TimelineCommandIDs::playPauseSequenceEditor,
		TimelineCommandIDs::prevCue,
		TimelineCommandIDs::nextCue,
		TimelineCommandIDs::prevTimeStep,
		TimelineCommandIDs::nextTimeStep,
		TimelineCommandIDs::goToStart,
		TimelineCommandIDs::goToEnd
	};

	commands.addArray(ids, numElementsInArray(ids));
}


void TimelineAppCommands::fillMenu(ApplicationCommandManager* commandManager, PopupMenu* menu, StringRef menuName)
{
	if (String(menuName) == "Timeline")
	{
		menu->addCommandItem(commandManager, TimelineCommandIDs::playPauseSequenceEditor);
		menu->addCommandItem(commandManager, TimelineCommandIDs::prevTimeStep);
		menu->addCommandItem(commandManager, TimelineCommandIDs::nextTimeStep);
		menu->addCommandItem(commandManager, TimelineCommandIDs::prevCue);
		menu->addCommandItem(commandManager, TimelineCommandIDs::nextCue);
	}
}

bool TimelineAppCommands::perform(const ApplicationCommandTarget::InvocationInfo& info)
{
	switch (info.commandID)
	{
	case TimelineCommandIDs::playPauseSequenceEditor:
	{
		Sequence* s = getCurrentEditingSequence();
		if (s != nullptr) s->togglePlayTrigger->trigger();
	}
	break;

	case TimelineCommandIDs::prevTimeStep:
	{
		Sequence* s = getCurrentEditingSequence();
		if (s != nullptr) s->currentTime->setValue(s->currentTime->floatValue() - (stepTime != nullptr ? stepTime->floatValue() : 1));
	}
	break;
	case TimelineCommandIDs::nextTimeStep:
	{
		Sequence* s = getCurrentEditingSequence();
		if (s != nullptr) s->currentTime->setValue(s->currentTime->floatValue() + (stepTime != nullptr ? stepTime->floatValue() : 1));
	}
	break;

	case TimelineCommandIDs::prevCue:
	{
		Sequence* s = getCurrentEditingSequence();
		if (s != nullptr) s->prevCue->trigger();

	}
	break;

	case TimelineCommandIDs::nextCue:
	{
		Sequence* s = getCurrentEditingSequence();
		if (s != nullptr) s->nextCue->trigger();
	}
	break;

	case TimelineCommandIDs::goToStart:
	{
		Sequence* s = getCurrentEditingSequence();
		if (s != nullptr) s->currentTime->setValue(0);
	}
	break;

	case TimelineCommandIDs::goToEnd:
	{
		Sequence* s = getCurrentEditingSequence();
		if (s != nullptr) s->currentTime->setValue(s->totalTime->floatValue());
	}
	break;
	}

	return true;
}

Sequence* TimelineAppCommands::getCurrentEditingSequence()
{
	ShapeShifterContent* sContent = ShapeShifterManager::getInstance()->getContentForName("Sequence Editor");
	if (sContent != nullptr)
	{
		Component* c = sContent->contentComponent;
		TimeMachineView* tmw = dynamic_cast<TimeMachineView*>(c);
		if (tmw != nullptr)
		{
			SequenceEditorView* se = tmw->editor.get();
			if (se != nullptr) return se->sequence;
		}
	}

	return nullptr;
}

StringArray TimelineAppCommands::getMenuBarNames()
{
	return StringArray("Timeline");
}
