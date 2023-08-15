#include "JuceHeader.h"

Metronome::Metronome(File bip, File bop)
{
	int numTics = 2;

	formatManager.registerBasicFormats() ;

	Array<const char*> ticData{ TimelineBinaryData::tic1_wav, TimelineBinaryData::tic2_wav };
	Array<int> ticSizes { TimelineBinaryData::tic1_wavSize, TimelineBinaryData::tic2_wavSize };

	Array<File> customFiles { bip, bop};

	for (int i = 0; i < numTics; i++)
	{
		AudioFormatReader* reader = nullptr;

		if (customFiles[i].existsAsFile())
		{
			std::unique_ptr<FileInputStream> s(new FileInputStream(customFiles[i]));
			reader = formatManager.createReaderFor(std::move(s));
		}
		else
		{
			std::unique_ptr<MemoryInputStream> ts(new MemoryInputStream(ticData[i], ticSizes[i], true));
			reader = formatManager.createReaderFor(std::move(ts));
		}

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
