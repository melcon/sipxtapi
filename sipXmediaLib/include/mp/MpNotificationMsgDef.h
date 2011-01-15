//  
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpNotificationMsgDef_h__
#define MpNotificationMsgDef_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS

// always add new enums to the end
typedef enum
{
   MP_NOTIFICATION_START_PLAY_FILE = 0,
   MP_NOTIFICATION_STOP_PLAY_FILE,
   MP_NOTIFICATION_START_PLAY_BUFFER,
   MP_NOTIFICATION_STOP_PLAY_BUFFER,
   MP_NOTIFICATION_PAUSE_PLAYBACK,
   MP_NOTIFICATION_RESUME_PLAYBACK,
   MP_NOTIFICATION_RECORDING_STARTED,
   MP_NOTIFICATION_RECORDING_STOPPED,
   MP_NOTIFICATION_DTMF_INBAND,
   MP_NOTIFICATION_DTMF_RFC2833,
   MP_NOTIFICATION_RESERVED1 // don't remove
} MpNotificationMsgType;

typedef enum
{
   MP_NOTIFICATION_AUDIO = 0,
   MP_NOTIFICATION_VIDEO
} MpNotificationMsgMedia;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS


#endif // MpNotificationMsgDef_h__
