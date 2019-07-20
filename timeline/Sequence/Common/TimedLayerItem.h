#pragma once

class TimedLayerItem
{
public:
	
	TimedLayerItem();
	virtual ~TimedLayerItem() {}
	
	float moveTimeReference;
	
	void setMoveTimeReference(bool setOtherSelectedItems = false);
	virtual void setMoveTimeReferenceInternal() = 0;

    void moveTime(float timeOffset, bool moveOtherSelectedItems = false);
	virtual void setTime(float time) = 0;

	void addMoveToUndoManager(bool addOtherSelectedItems = false);

	virtual UndoableAction* getUndoableMoveAction() = 0;


};