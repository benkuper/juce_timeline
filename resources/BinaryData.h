/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace TimelineBinaryData
{
    extern const char*   nextcue_png;
    const int            nextcue_pngSize = 2624;

    extern const char*   padlock_png;
    const int            padlock_pngSize = 584;

    extern const char*   play_png;
    const int            play_pngSize = 1849;

    extern const char*   prevcue_png;
    const int            prevcue_pngSize = 2536;

    extern const char*   smallstripe_png;
    const int            smallstripe_pngSize = 1959;

    extern const char*   snap_png;
    const int            snap_pngSize = 3649;

    extern const char*   stop_png;
    const int            stop_pngSize = 1978;

    extern const char*   stripe_png;
    const int            stripe_pngSize = 2043;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 8;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
