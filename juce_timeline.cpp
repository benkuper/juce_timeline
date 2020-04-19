/*
  ==============================================================================

   

  ==============================================================================
*/

#ifdef ORGANIC_UI_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#include "juce_timeline.h"

//==============================================================================

#include "resources/BinaryData.cpp"

#include "timeline/Sequence/Sequence.cpp"
#include "timeline/SequenceManager.cpp"

#include "timeline/Sequence/Layer/layers/Block/LayerBlock.cpp"
#include "timeline/Sequence/Layer/layers/Block/LayerBlockManager.cpp"

#include "timeline/Sequence/Layer/layers/audio/AudioLayerClip.cpp"
#include "timeline/Sequence/Layer/layers/audio/AudioLayerClipManager.cpp"
#include "timeline/Sequence/Layer/layers/audio/AudioLayer.cpp"

#include "timeline/Sequence/Layer/layers/Trigger/TimeTrigger.cpp"
#include "timeline/Sequence/Layer/layers/Trigger/TimeTriggerManager.cpp"
#include "timeline/Sequence/Layer/layers/Trigger/TriggerLayer.cpp"

#include "timeline/Sequence/Layer/SequenceLayer.cpp"
#include "timeline/Sequence/Layer/SequenceLayerManager.cpp"

#include "timeline/Sequence/Cue/TimeCue.cpp"
#include "timeline/Sequence/Cue/TimeCueManager.cpp"
 

//ui
#include "timeline/Sequence/Layer/ui/SequenceLayerPanel.cpp"
#include "timeline/Sequence/Layer/ui/SequenceLayerPanelManagerUI.cpp"
#include "timeline/Sequence/Layer/ui/SequenceLayerTimeline.cpp"
#include "timeline/Sequence/Layer/ui/SequenceLayerTimelineManagerUI.cpp"

#include "timeline/Sequence/Layer/layers/Block/ui/LayerBlockUI.cpp"
#include "timeline/Sequence/Layer/layers/Block/ui/LayerBlockManagerUI.cpp"

#include "timeline/Sequence/Layer/layers/audio/ui/AudioLayerClipUI.cpp"
#include "timeline/Sequence/Layer/layers/audio/ui/AudioLayerClipManagerUI.cpp"
#include "timeline/Sequence/Layer/layers/audio/ui/AudioLayerPanel.cpp"
#include "timeline/Sequence/Layer/layers/audio/ui/AudioLayerTimeline.cpp"


#include "timeline/Sequence/Layer/layers/Trigger/ui/TimeTriggerUI.cpp"
#include "timeline/Sequence/Layer/layers/Trigger/ui/TimeTriggerMultiTransformer.cpp"
#include "timeline/Sequence/Layer/layers/Trigger/ui/TriggerLayerPanel.cpp"
#include "timeline/Sequence/Layer/layers/Trigger/ui/TriggerLayerTimeline.cpp"
#include "timeline/Sequence/Layer/layers/Trigger/ui/TimeTriggerManagerUI.cpp"

#include "timeline/Sequence/ui/SequenceUI.cpp"
#include "timeline/Sequence/ui/SequenceTransportUI.cpp"
#include "timeline/Sequence/ui/SequenceTimelineHeader.cpp"
#include "timeline/Sequence/ui/SequenceTimelineNavigationUI.cpp"
#include "timeline/Sequence/ui/SequenceTimelineSeeker.cpp"
#include "timeline/Sequence/ui/SequenceEditorView.cpp"

#include "timeline/Sequence/Cue/ui/TimeCueUI.cpp"
#include "timeline/Sequence/Cue/ui/TimeCueManagerUI.cpp"

#include "timeline/ui/TimeMachineView.cpp"
#include "timeline/ui/SequenceManagerUI.cpp"


#include "TimelineAppCommands.cpp"