//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef SipXConference_h__
#define SipXConference_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include "tapi/SipXCore.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
class SIPX_INSTANCE_DATA;
class OsStackTraceLogger;

// STRUCTS
// TYPEDEFS
class SIPX_CONF_DATA
{
public:
   UtlString           confCallId;
   SIPX_INSTANCE_DATA* pInst;
   size_t              nCalls;
   SIPX_CALL           hCalls[CONF_MAX_CONNECTIONS];
   CONF_HOLD_STATE     confHoldState;
   SIPX_TRANSPORT      hTransport;
   int                 nNumFilesPlaying;
   OsMutex             mutex;
   
   SIPX_CONF_DATA() : pInst(NULL),
      confCallId(NULL),
      nCalls(0),
      confHoldState(CONF_STATE_UNHELD),
      hTransport(0),
      nNumFilesPlaying(0),
      mutex(OsMutex::Q_FIFO)
   {
      memset(hCalls, 0, sizeof(SIPX_CALL) * CONF_MAX_CONNECTIONS);
   }

};

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

SIPX_CONF sipxConfLookupHandleByConfId(const UtlString& confID, SIPX_INST pInst);

SIPX_CONF_DATA* sipxConfLookup(const SIPX_CONF hConf,
                               SIPX_LOCK_TYPE type,
                               const OsStackTraceLogger& oneBackInStack);

void sipxConfReleaseLock(SIPX_CONF_DATA* pData,
                         SIPX_LOCK_TYPE type,
                         const OsStackTraceLogger& oneBackInStack);

void sipxConfFree(const SIPX_CONF hConf);

void sipxConferenceDestroyAll(const SIPX_INST hInst);

UtlBoolean validConfData(const SIPX_CONF_DATA* pData);

UtlBoolean sipxAddCallHandleToConf(const SIPX_CALL hCall,
                                   const SIPX_CONF hConf);

// WARNING: This relies on outside locking of conference SIPX_CONF_DATA
UtlBoolean sipxRemoveCallHandleFromConf(const SIPX_CONF hConf,
                                        const SIPX_CALL hCall);
#endif // SipXConference_h__
