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
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

