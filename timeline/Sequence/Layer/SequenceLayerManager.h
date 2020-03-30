/*
  ==============================================================================

    SequenceLayerManager.h
    Created: 28 Oct 2016 8:15:28pm
    Author:  bkupe

  ==============================================================================
*/
#pragma once

class SequenceLayerManager :
	public BaseManager<SequenceLayer>
{
public:
	SequenceLayerManager(Sequence * _sequence);
	~SequenceLayerManager();

	Sequence * sequence;
	Factory<SequenceLayer> factory;
	
	virtual void fileDropped(String file);

	class LayerDefinition : 
		public FactoryParametricDefinition<SequenceLayer, std::function<SequenceLayer *(Sequence *, var)>>
	{
	public:
		LayerDefinition(StringRef menu, StringRef type, std::function<SequenceLayer *(Sequence*, var)> func, Sequence* s, bool isAudio = false) :
			FactoryParametricDefinition(menu, type, func),
			sequence(s),
			isAudio(isAudio)
		{
		}

		static LayerDefinition* createDef(StringRef menu, StringRef type, std::function<SequenceLayer* (Sequence*, var)> func, Sequence* s, bool isAudio = false)
		{
			return new LayerDefinition(menu, type, func, s, isAudio);
		}

		virtual ~LayerDefinition() {}

		Sequence* sequence;
		bool isAudio;
		SequenceLayer* create() override { return createFunc(sequence, params); }
	};


	SequenceLayer * createItem() override;
	virtual void createAudioLayerForFile(File f);

#if TIMELINE_UNIQUE_LAYER_FACTORY
	SequenceLayer * addItemFromData(var data, bool fromUndoableAction = false) override;
#endif

	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceLayerManager)

};