/*
  ==============================================================================

  ==============================================================================
*/


/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.txt file.


 BEGIN_JUCE_MODULE_DECLARATION

  ID:               juce_timeline
  vendor:           benkuper
  version:          1.0.0
  name:             Organic Timeline
  description:      A timeline framework that uses OrganicUI
  website:          https://github.com/benkuper/juce_timeline
  license:          GPLv3

  dependencies:    juce_organicui, juce_audio_basics,juce_audio_devices,juce_audio_formats, juce_audio_processors, juce_audio_utils

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/

#pragma once
#define ORGANIC_TIMELINE_H_INCLUDED

//==============================================================================

#ifdef _MSC_VER
 #pragma warning (push)
 // Disable warnings for long class names, padding, and undefined preprocessor definitions.
 #pragma warning (disable: 4251 4786 4668 4820)
#endif


/** Config: TIMELINE_USE_SEQUENCEMANAGER_SINGLETON
	Enables the declaration of SequenceManager as a Singleton. If you have special handling of your Sequence Manager, or want mutiple managers, you should not use this
*/
#ifndef TIMELINE_USE_SEQUENCEMANAGER_SINGLETON
#define TIMELINE_USE_SEQUENCEMANAGER_SINGLETON 0
#endif



#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_organicui/juce_organicui.h>

//Somehow, we need to include specifically the templace class headers
#include <juce_organicui/undo/UndoMaster.h>
#include <juce_organicui/manager/BaseItemListener.h>
#include <juce_organicui/manager/BaseManagerListener.h>
#include <juce_organicui/manager/BaseItem.h>
#include <juce_organicui/manager/ui/BaseItemEditor.h>
#include <juce_organicui/manager/ui/GenericManagerEditor.h>
#include <juce_organicui/manager/BaseManager.h>
#include <juce_organicui/manager/ui/BaseItemMinimalUI.h>
#include <juce_organicui/manager/ui/BaseItemUI.h>
#include <juce_organicui/manager/ui/BaseManagerUI.h>
#include <juce_organicui/manager/ui/BaseManagerViewUI.h>
#include <juce_organicui/manager/ui/BaseManagerShapeShifterUI.h>
#include <juce_organicui/ui/shapeshifter/GapGrabber.h>


using namespace juce;

#include "resources/BinaryData.h"

#include "timeline/Sequence/Sequence.h"
#include "timeline/SequenceManager.h"

#include "timeline/Sequence/Layer/SequenceLayer.h"

#include "timeline/Sequence/Common/TimedLayerItem.h"

#include "timeline/Sequence/Layer/layers/Block/LayerBlock.h"
#include "timeline/Sequence/Layer/layers/Block/LayerBlockManager.h"

#include "timeline/Sequence/Layer/layers/audio/AudioLayerClip.h"
#include "timeline/Sequence/Layer/layers/audio/AudioLayerClipManager.h"
#include "timeline/Sequence/Layer/layers/audio/AudioLayer.h"


#include "timeline/Sequence/Layer/layers/Trigger/TimeTrigger.h"
#include "timeline/Sequence/Layer/layers/Trigger/TimeTriggerManager.h"
#include "timeline/Sequence/Layer/layers/Trigger/TriggerLayer.h"

#include "timeline/Sequence/Layer/SequenceLayerManager.h"

#include "timeline/Sequence/Cue/TimeCue.h"
#include "timeline/Sequence/Cue/TimeCueManager.h"

//ui

#include "timeline/Sequence/Layer/ui/SequenceLayerPanel.h"
#include "timeline/Sequence/Layer/ui/SequenceLayerPanelManagerUI.h"
#include "timeline/Sequence/Layer/ui/SequenceLayerTimeline.h"
#include "timeline/Sequence/Layer/ui/SequenceLayerTimelineManagerUI.h"

#include "timeline/Sequence/Layer/layers/Block/ui/LayerBlockUI.h"
#include "timeline/Sequence/Layer/layers/Block/ui/LayerBlockManagerUI.h"

#include "timeline/Sequence/Layer/layers/audio/ui/AudioLayerClipUI.h"
#include "timeline/Sequence/Layer/layers/audio/ui/AudioLayerClipManagerUI.h"
#include "timeline/Sequence/Layer/layers/audio/ui/AudioLayerPanel.h"
#include "timeline/Sequence/Layer/layers/audio/ui/AudioLayerTimeline.h"

#include "timeline/Sequence/Layer/layers/Trigger/ui/TimeTriggerUI.h"
#include "timeline/Sequence/Layer/layers/Trigger/ui/TimeTriggerMultiTransformer.h"
#include "timeline/Sequence/Layer/layers/Trigger/ui/TriggerLayerPanel.h"
#include "timeline/Sequence/Layer/layers/Trigger/ui/TriggerLayerTimeline.h"
#include "timeline/Sequence/Layer/layers/Trigger/ui/TimeTriggerManagerUI.h"

#include "timeline/Sequence/Cue/ui/TimeCueUI.h"
#include "timeline/Sequence/Cue/ui/TimeCueManagerUI.h"

#include "timeline/Sequence/ui/SequenceUI.h"
#include "timeline/Sequence/ui/SequenceTransportUI.h"
#include "timeline/Sequence/ui/SequenceTimelineSeeker.h"
#include "timeline/Sequence/ui/SequenceTimelineHeader.h"
#include "timeline/Sequence/ui/SequenceTimelineNavigationUI.h"
#include "timeline/Sequence/ui/SequenceEditorView.h"

#include "timeline/ui/TimeMachineView.h"
#include "timeline/ui/SequenceManagerUI.h"
