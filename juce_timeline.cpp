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

#include "timeline/Sequence/Metronome.cpp"
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


#include "timeline/Sequence/Layer/layers/SequenceBlock/SequenceBlock.cpp"
#include "timeline/Sequence/Layer/layers/SequenceBlock/SequenceBlockManager.cpp"
#include "timeline/Sequence/Layer/layers/SequenceBlock/SequenceBlockLayer.cpp"

#include "timeline/Sequence/Layer/SequenceLayer.cpp"
#include "timeline/Sequence/Layer/SequenceLayerGroup.cpp"
#include "timeline/Sequence/Layer/SequenceLayerManager.cpp"

#include "timeline/Sequence/Cue/TimeCue.cpp"
#include "timeline/Sequence/Cue/TimeCueManager.cpp"
 