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

#ifndef CpNotificationMsgDef_h__
#define CpNotificationMsgDef_h__

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
// @note Keep in sync with MpNotificationMsgType
typedef enum
{
   CP_NOTIFICATION_START_PLAY_FILE = 0,
   CP_NOTIFICATION_STOP_PLAY_FILE,
   CP_NOTIFICATION_START_PLAY_BUFFER,
   CP_NOTIFICATION_STOP_PLAY_BUFFER,
   CP_NOTIFICATION_RECORDING_STARTED,
   CP_NOTIFICATION_RECORDING_STOPPED,
   CP_NOTIFICATION_DTMF
} CpNotificationMsgType;

typedef enum
{
   CP_NOTIFICATION_AUDIO = 0,
   CP_NOTIFICATION_VIDEO
} CpNotificationMsgMedia;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS


#endif // CpNotificationMsgDef_h__
