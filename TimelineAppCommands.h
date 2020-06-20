#pragma once

class TimelineAppCommands
{
public:
	static void getAllCommands(Array<CommandID>& commands);
	static void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result);
	static void fillMenu(ApplicationCommandManager* commandManager, PopupMenu* menu, StringRef menuName);
	static bool perform(const ApplicationCommandTarget::InvocationInfo& info);
	static Sequence* getCurrentEditingSequence();
	static StringArray getMenuBarNames();
};
