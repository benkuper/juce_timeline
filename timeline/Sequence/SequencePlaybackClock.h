#pragma once

#include <cmath>
#include <cstdint>

// Maps a monotonic wall clock to sequence frames. A late wake may skip frames,
// but an early wake never evaluates the same frame twice.
class SequencePlaybackClock
{
public:
	void reset(double wallMillis, double sequenceSeconds, int framesPerSecond, double playbackSpeed)
	{
		originMillis = wallMillis;
		originSeconds = sequenceSeconds;
		fps = framesPerSecond > 0 ? framesPerSecond : 1;
		speed = playbackSpeed;
	}

	std::int64_t frameAt(double wallMillis) const
	{
		const double frames = (wallMillis - originMillis) * fps / 1000.0;
		return frames > 0.0 ? static_cast<std::int64_t>(std::floor(frames + 1.0e-12)) : 0;
	}

	double deadlineForFrame(std::int64_t frame) const
	{
		return originMillis + static_cast<double>(frame) * 1000.0 / fps;
	}

	double timeForFrame(std::int64_t frame) const
	{
		return originSeconds + static_cast<double>(frame) * speed / fps;
	}

	int getFramesPerSecond() const { return fps; }
	double getSpeed() const { return speed; }

private:
	double originMillis = 0.0;
	double originSeconds = 0.0;
	int fps = 1;
	double speed = 1.0;
};
