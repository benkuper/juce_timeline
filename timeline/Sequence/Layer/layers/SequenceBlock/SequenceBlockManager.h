/*
  ==============================================================================

    SequenceBlockManager.h
    Created: 8 Feb 2017 12:19:53pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class SequenceBlockLayer;
class SequenceManager;

class SequenceBlockManager :
	public LayerBlockManager
{
public:
	SequenceBlockManager(SequenceBlockLayer * layer, SequenceManager * refManager);
	~SequenceBlockManager();

	SequenceManager* refManager;

	SequenceBlockLayer * sequenceBlockLayer;
	LayerBlock* createItem() override;
	
};
