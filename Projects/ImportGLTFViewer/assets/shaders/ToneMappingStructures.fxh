#ifndef _TONE_MAPPING_STRUCTURES_FXH_
#define _TONE_MAPPING_STRUCTURES_FXH_

#ifdef __cplusplus

#   ifndef BOOL
#      define BOOL int32_t // Do not use bool, because sizeof(bool)==1 !
#   endif

#   ifndef TRUE
#      define TRUE 1
#   endif

#   ifndef CHECK_STRUCT_ALIGNMENT
        // Note that defining empty macros causes GL shader compilation error on Mac, because
        // it does not allow standalone semicolons outside of main.
        // On the other hand, adding semicolon at the end of the macro definition causes gcc error.
#       define CHECK_STRUCT_ALIGNMENT(s) static_assert( sizeof(s) % 16 == 0, "sizeof(" #s ") is not multiple of 16" )
#   endif

#   ifndef DEFAULT_VALUE
#       define DEFAULT_VALUE(x) =x
#   endif

#else

#   ifndef BOOL
#       define BOOL bool
#   endif

#   ifndef DEFAULT_VALUE
#       define DEFAULT_VALUE(x)
#   endif

#endif


// Tone mapping mode
#define TONE_MAPPING_MODE_EXP           0
#define TONE_MAPPING_MODE_REINHARD      1
#define TONE_MAPPING_MODE_REINHARD_MOD  2
#define TONE_MAPPING_MODE_UNCHARTED2    3
#define TONE_MAPPING_FILMIC_ALU         4
#define TONE_MAPPING_LOGARITHMIC        5
#define TONE_MAPPING_ADAPTIVE_LOG       6

struct ToneMappingAttribs
{
    // Tone mapping mode.
    int   iToneMappingMode                  DEFAULT_VALUE(TONE_MAPPING_MODE_UNCHARTED2);
    // Automatically compute exposure to use in tone mapping.
    BOOL  bAutoExposure                     DEFAULT_VALUE(TRUE);
    // Middle gray value used by tone mapping operators.
    float fMiddleGray                       DEFAULT_VALUE(0.18f);
    // Simulate eye adaptation to light changes.
    BOOL  bLightAdaptation                  DEFAULT_VALUE(TRUE);

    // White point to use in tone mapping.
    float fWhitePoint                       DEFAULT_VALUE(3.f);
    // Luminance point to use in tone mapping.
    float fLuminanceSaturation              DEFAULT_VALUE(1.f);
    uint Padding0;
    uint Padding1;
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(ToneMappingAttribs);
#endif

#endif // _TONE_MAPPING_STRUCTURES_FXH_
