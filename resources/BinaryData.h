/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace TimelineBinaryData
{
    extern const char*   nextcue_png;
    const int            nextcue_pngSize = 877;

    extern const char*   pause_png;
    const int            pause_pngSize = 701;

    extern const char*   play_png;
    const int            play_pngSize = 768;

    extern const char*   playing_png;
    const int            playing_pngSize = 952;

    extern const char*   prevcue_png;
    const int            prevcue_pngSize = 1043;

    extern const char*   stop_png;
    const int            stop_pngSize = 813;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 6;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) noexcept;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8) noexcept;
}
