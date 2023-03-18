#include "JuceHeader.h"

Metronome::Metronome()
{

	WavAudioFormat wavFormat;

	Array<const char*> ticData{ TimelineBinaryData::tic1_wav ,TimelineBinaryData::tic2_wav };
	Array<int> ticSizes{ TimelineBinaryData::tic1_wavSize ,TimelineBinaryData::tic2_wavSize };

	for (int i = 0; i < ticData.size(); i++)
	{
		MemoryInputStream* ts(new MemoryInputStream(ticData[i], ticSizes[i], true));
		AudioFormatReader* reader(wavFormat.createReaderFor(ts, true));
		
		AudioFormatReaderSource* newSource = new AudioFormatReaderSource(reader, true);
		AudioTransportSource* transport = new AudioTransportSource();
		ChannelRemappingAudioSource* channelRemap = new ChannelRemappingAudioSource(transport, false);

		transport->setSource(newSource, 0, nullptr, reader->sampleRate, reader->numChannels);
		ticTransports.add(transport);
		ticReaders.add(newSource);
		ticChannelRemap.add(channelRemap);
		
	}

}

Metronome::~Metronome()
{
}
