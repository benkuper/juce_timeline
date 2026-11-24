#pragma once

#include <algorithm>
#include <cmath>

// Timeline-only math shared by audio and value blocks. No live parameter reads.
namespace BlockTransitions
{
struct Fades { double in = 0, out = 0; };

inline double overlap(double start, double end, double otherStart, double otherEnd)
{
    return std::max(0.0, std::min(end, otherEnd) - std::max(start, otherStart));
}

inline Fades fit(Fades fades, double length)
{
    fades.in = std::max(0.0, fades.in); fades.out = std::max(0.0, fades.out);
    if (fades.in + fades.out > length && fades.in + fades.out > 0)
    {
        const double scale = std::max(0.0, length) / (fades.in + fades.out);
        fades.in *= scale; fades.out *= scale;
    }
    return fades;
}

inline double gain(double localTime, double length, Fades fades)
{
    const double in = fades.in > 0 ? std::clamp(localTime / fades.in, 0.0, 1.0) : 1.0;
    const double out = fades.out > 0 ? std::clamp((length - localTime) / fades.out, 0.0, 1.0) : 1.0;
    return std::min(in, out);
}

inline Fades withReservedOverlaps(Fades manual, double length, Fades automatic)
{
    automatic = fit(automatic, length);
    manual = fit(manual, std::max(0.0, length - automatic.in - automatic.out));
    return { manual.in + automatic.in, manual.out + automatic.out };
}
}
