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
	
	class LayerDefinition : 
		public FactoryParametricDefinition<SequenceLayer, std::function<SequenceLayer *(Sequence *, var)>>
	{
	public:
		LayerDefinition(StringRef menu, StringRef type, std::function<SequenceLayer *(Sequence*, var)> func, Sequence* s) :
			FactoryParametricDefinition(menu, type, func),
			sequence(s)
		{
		}

		static LayerDefinition* createDef(StringRef menu, StringRef type, std::function<SequenceLayer* (Sequence*, var)> func, Sequence* s)
		{
			return new LayerDefinition(menu, type, func, s);
		}

		virtual ~LayerDefinition() {}

		Sequence* sequence;
		SequenceLayer* create() override { return createFunc(sequence, params); }
	};


	SequenceLayer * createItem() override;

#if TIMELINE_UNIQUE_LAYER_FACTORY
	SequenceLayer * addItemFromData(var data, bool fromUndoableAction = false) override;
#endif

	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceLayerManager)

};