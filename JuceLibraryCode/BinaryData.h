/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   Metropolitan_ttf;
    const int            Metropolitan_ttfSize = 55652;

    extern const char*   SaturationON_png;
    const int            SaturationON_pngSize = 16392;

    extern const char*   SaturationOFF_png;
    const int            SaturationOFF_pngSize = 17661;

    extern const char*   backgroundmodern_png;
    const int            backgroundmodern_pngSize = 681427;

    extern const char*   upheavtt_ttf;
    const int            upheavtt_ttfSize = 41836;

    extern const char*   black_panel_png;
    const int            black_panel_pngSize = 920167;

    extern const char*   pinkknob_png;
    const int            pinkknob_pngSize = 288277;

    extern const char*   blackknob_png;
    const int            blackknob_pngSize = 653726;

    extern const char*   whiteknob_png;
    const int            whiteknob_pngSize = 278532;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 9;

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
