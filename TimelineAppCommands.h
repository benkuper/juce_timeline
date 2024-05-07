#pragma once
namespace TimelineCommandIDs
{
	const int playPauseSequenceEditor = 0x80000;
	const int prevCue = 0x80001;
	const int nextCue = 0x80002;
	const int prevTimeStep = 0x80003;
	const int nextTimeStep = 0x80004;
	const int goToStart = 0x80005;
	const int goToEnd = 0x80006;
	const int addCueAtPosition = 0x80007;
	const int moveOneFrameAfter = 0x80008;
	const int moveOneFrameBefore = 0x80009;
}

class TimelineAppCommands
{
public:
	static bool useSpaceBarAsPlayPause;
	static void getAllCommands(Array<CommandID>& commands);
	static void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result);
	static void fillMenu(ApplicationCommandManager* commandManager, PopupMenu* menu, StringRef menuName);
	static bool perform(const ApplicationCommandTarget::InvocationInfo& info);
	static Sequence* getCurrentEditingSequence();
	static StringArray getMenuBarNames();
};
