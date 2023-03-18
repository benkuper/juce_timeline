/*
  ==============================================================================

    Sequence.h
    Created: 28 Oct 2016 8:13:19pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class Metronome
{
public:
    Metronome();
    ~Metronome();

    OwnedArray<AudioFormatReaderSource> ticReaders;
    OwnedArray<AudioTransportSource> ticTransports;
    OwnedArray<ChannelRemappingAudioSource> ticChannelRemap;
};