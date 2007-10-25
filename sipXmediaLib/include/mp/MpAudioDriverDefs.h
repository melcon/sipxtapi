//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpAudioDriverDefs_h__
#define MpAudioDriverDefs_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES

/**
* Use for MpAudioDriverSampleFormat
*/
#define MP_AUDIO_FORMAT_FLOAT32        0x00000001
#define MP_AUDIO_FORMAT_INT32          0x00000002
#define MP_AUDIO_FORMAT_INT24          0x00000004
#define MP_AUDIO_FORMAT_INT16          0x00000008
#define MP_AUDIO_FORMAT_INT8           0x00000010
#define MP_AUDIO_FORMAT_UINT8          0x00000020
#define MP_AUDIO_FORMAT_CUSTOMFORMAT   0x00010000
#define MP_AUDIO_FORMAT_NONINTERLEAVED 0x80000000

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS

typedef unsigned long MpAudioDriverSampleFormat;
typedef int MpAudioDeviceIndex;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

#endif // MpAudioDriverDefs_h__
