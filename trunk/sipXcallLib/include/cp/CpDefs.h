//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef CpDefs_h__
#define CpDefs_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES

#ifdef LONG_EVENT_RESPONSE_TIMEOUTS
#  define CP_MAX_EVENT_WAIT_SECONDS    2592000    // 30 days in seconds
#else
#  define CP_MAX_EVENT_WAIT_SECONDS    30         // time out, seconds
#endif

#define CP_CALL_HISTORY_LENGTH 50

#define CP_MAXIMUM_RINGING_EXPIRE_SECONDS 180

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS

typedef int CP_CONTACT_ID; 

/**
* DTMF/other tone ids.
*/
typedef enum CP_TONE_ID
{
   CP_ID_DTMF_0 = 0,
   CP_ID_DTMF_1 = 1,
   CP_ID_DTMF_2 = 2,
   CP_ID_DTMF_3 = 3,
   CP_ID_DTMF_4 = 4,
   CP_ID_DTMF_5 = 5,
   CP_ID_DTMF_6 = 6,
   CP_ID_DTMF_7 = 7,
   CP_ID_DTMF_8 = 8,
   CP_ID_DTMF_9 = 9,
   CP_ID_DTMF_STAR = 10,
   CP_ID_DTMF_POUND = 11,
   CP_ID_DTMF_A = 12,
   CP_ID_DTMF_B = 13,
   CP_ID_DTMF_C = 14,
   CP_ID_DTMF_D = 15,
   CP_ID_DTMF_FLASH = 16,
   CP_ID_TONE_DIALTONE = 512,
   CP_ID_TONE_BUSY,
   CP_ID_TONE_RINGBACK,
   CP_ID_TONE_RINGTONE,
   CP_ID_TONE_CALLFAILED,
   CP_ID_TONE_SILENCE,
   CP_ID_TONE_BACKSPACE,
   CP_ID_TONE_CALLWAITING,
   CP_ID_TONE_CALLHELD,
   CP_ID_TONE_LOUD_FAST_BUSY
} CP_TONE_ID;

typedef enum
{
   CP_MEDIA_TYPE_AUDIO = 0,
   CP_MEDIA_TYPE_VIDEO,
} CP_MEDIA_TYPE;

typedef enum
{
   CP_MEDIA_CAUSE_NORMAL = 0,
   CP_MEDIA_CAUSE_HOLD,
   CP_MEDIA_CAUSE_UNHOLD,
   CP_MEDIA_CAUSE_FAILED,
   CP_MEDIA_CAUSE_DEVICE_UNAVAILABLE,
   CP_MEDIA_CAUSE_INCOMPATIBLE,
   CP_MEDIA_CAUSE_DTMF_INBAND,
   CP_MEDIA_CAUSE_DTMF_RFC2833,
   CP_MEDIA_CAUSE_DTMF_SIPINFO
} CP_MEDIA_CAUSE;

typedef enum
{
   CP_MEDIA_UNKNOWN = 0,
   CP_MEDIA_LOCAL_START = 50000,
   CP_MEDIA_LOCAL_STOP, 
   CP_MEDIA_REMOTE_START,
   CP_MEDIA_REMOTE_STOP,
   CP_MEDIA_REMOTE_SILENT,
   CP_MEDIA_PLAYFILE_START,
   CP_MEDIA_PLAYFILE_STOP,
   CP_MEDIA_PLAYBUFFER_START,
   CP_MEDIA_PLAYBUFFER_STOP,
   CP_MEDIA_PLAYBACK_PAUSED,
   CP_MEDIA_PLAYBACK_RESUMED,
   CP_MEDIA_REMOTE_DTMF,
   CP_MEDIA_DEVICE_FAILURE,
   CP_MEDIA_REMOTE_ACTIVE,
   CP_MEDIA_RECORDING_START,
   CP_MEDIA_RECORDING_STOP
} CP_MEDIA_EVENT;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

#endif // CpDefs_h__
