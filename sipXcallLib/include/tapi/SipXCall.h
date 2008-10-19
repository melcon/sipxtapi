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

#ifndef SipXCall_h__
#define SipXCall_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include <net/Url.h>
#include "tapi/sipXtapi.h"
#include "tapi/sipXtapiEvents.h"
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
class SIPX_CALL_DATA
{
public:
   UtlString callId; ///< Id identifying CpPeerCall instance
   UtlString sessionCallId; ///< SIP CallId used in SIP Messages, identifies SipConnection
   UtlString ghostCallId;
   UtlString remoteAddress;
   UtlString fromURI; ///< from URI used for call, will contain tag. For outbound calls tag is added later.
   Url lineURI; ///< URI of line. Copy of m_lineURI from SIPX_LINE_DATA. This one will never contain a tag.
   UtlString remoteContactAddress;///< Remote Contact URI
   SIPX_LINE hLine;
   SIPX_INSTANCE_DATA* pInst;
   OsMutex pMutex;
   SIPX_CONF hConf;
   SIPX_SECURITY_ATTRIBUTES security;
   SIPX_VIDEO_DISPLAY display;
   UtlBoolean bRemoveInsteadOfDrop;   /** Remove the call instead of dropping it 
                                       -- this is used as part of consultative 
                                       transfer when we are the transfer target 
                                       and need to replace a call leg within 
                                       the same CpPeerCall. */
   SIPX_CALLSTATE_EVENT lastCallstateEvent;
   SIPX_CALLSTATE_CAUSE lastCallstateCause;

   SIPX_MEDIA_EVENT lastLocalMediaAudioEvent;
   SIPX_MEDIA_EVENT lastLocalMediaVideoEvent;
   SIPX_MEDIA_EVENT lastRemoteMediaAudioEvent;
   SIPX_MEDIA_EVENT lastRemoteMediaVideoEvent;

   SIPX_INTERNAL_CALLSTATE state;
   UtlBoolean bInFocus;
   int connectionId;                  /** Cache the connection id */
   SIPX_TRANSPORT hTransport;
   bool bHoldAfterConnect;            /** Used if we are the transfer target, and the
                                      replaced call is HELD or REMOTE_HELD, then
                                      this flag is set, and indicates that the call
                                      should be placed on hold after the connection
                                      is established. */
   UtlBoolean bCallHoldInvoked;             /** Set to true if sipxCallHold has been invoked.
                                      Set to fales if sipxCallUnhold has been invoked. */                                          
   SIPX_CALL_DATA()
      : pMutex(OsMutex::Q_FIFO),
      callId(NULL),
      sessionCallId(NULL),
      ghostCallId(NULL),
      remoteAddress(NULL),
      fromURI(NULL),
      lineURI(),
      remoteContactAddress(NULL),
      hLine(0),
      pInst(NULL),
      hConf(NULL),
      security(),
      display(),
      bRemoveInsteadOfDrop(false),
      lastCallstateEvent(CALLSTATE_UNKNOWN),
      lastCallstateCause(CALLSTATE_CAUSE_UNKNOWN),
      lastLocalMediaAudioEvent(MEDIA_UNKNOWN),
      lastLocalMediaVideoEvent(MEDIA_UNKNOWN),
      lastRemoteMediaAudioEvent(MEDIA_UNKNOWN),
      lastRemoteMediaVideoEvent(MEDIA_UNKNOWN),
      state(SIPX_INTERNAL_CALLSTATE_UNKNOWN),
      bInFocus(false),
      connectionId(0),
      hTransport(0),
      bHoldAfterConnect(false),
      bCallHoldInvoked(false)
   {

   }
};

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

void sipxCallReleaseLock(SIPX_CALL_DATA* pData,
                         SIPX_LOCK_TYPE type,
                         const OsStackTraceLogger& oneBackInStack);

UtlBoolean validCallData(SIPX_CALL_DATA* pData);

SIPX_CALL_DATA* sipxCallLookup(const SIPX_CALL hCall,
                               SIPX_LOCK_TYPE type,
                               const OsStackTraceLogger& oneBackInStack);

void sipxCallObjectFree(const SIPX_CALL hCall, const OsStackTraceLogger& oneBackInStack);

SIPX_CALL sipxCallLookupHandleBySessionCallId(const UtlString& sessionCallID, SIPX_INST pInst);
SIPX_CALL sipxCallLookupHandleByCallId(const UtlString& callID, SIPX_INST pInst);

void destroyCallData(SIPX_CALL_DATA* pData);

UtlBoolean sipxCallGetMediaState(SIPX_CALL hCall,
                                 SIPX_MEDIA_EVENT& lastLocalMediaAudioEvent,
                                 SIPX_MEDIA_EVENT& lastLocalMediaVideoEvent,
                                 SIPX_MEDIA_EVENT& lastRemoteMediaAudioEvent,
                                 SIPX_MEDIA_EVENT& lastRemoteMediaVideoEvent);

UtlBoolean sipxCallSetMediaState(SIPX_CALL hCall,
                                 SIPX_MEDIA_EVENT event,
                                 SIPX_MEDIA_TYPE type);

UtlBoolean sipxCallGetState(SIPX_CALL hCall, 
                            SIPX_CALLSTATE_EVENT& lastEvent,
                            SIPX_CALLSTATE_CAUSE& lastCause,
                            SIPX_INTERNAL_CALLSTATE& state);

UtlBoolean sipxCallGetCommonData(SIPX_CALL hCall,
                                 SIPX_INSTANCE_DATA** pInst,
                                 UtlString* pCallId,
                                 UtlString* pSessionCallId,
                                 UtlString* pStrRemoteAddress,
                                 UtlString* pLineUri,
                                 UtlString* pGhostCallId = NULL, 
                                 UtlString* pRemoteContactAddress = NULL);

/**
* Get the list of active calls for the specified call manager instance
*/
SIPXTAPI_API SIPX_RESULT sipxGetAllCallIds(SIPX_INST hInst, UtlSList& sessionCallIdList);

void sipxCallDestroyAll(const SIPX_INST hInst);

SIPX_CONF sipxCallGetConf(SIPX_CALL hCall);

SIPX_CONTACT_TYPE sipxCallGetLineContactType(SIPX_CALL hCall);

UtlBoolean sipxCallSetRemoveInsteadofDrop(SIPX_CALL hCall);

UtlBoolean sipxCallIsRemoveInsteadOfDropSet(SIPX_CALL hCall);

SIPX_RESULT sipxCallCreateHelper(const SIPX_INST hInst,
                                 const SIPX_LINE hLine,
                                 const char* szLine,
                                 const SIPX_CONF hConf,
                                 SIPX_CALL* phCall,
                                 const UtlString& sCallId = "",
                                 const UtlString& sSessionCallId = "",
                                 bool bCreateInCallManager = true,
                                 bool bFireDialtone = true);


SIPX_RESULT sipxCallDrop(SIPX_CALL& hCall);

UtlBoolean sipxCallSetState(SIPX_CALL hCall, 
                            SIPX_CALLSTATE_EVENT event,
                            SIPX_CALLSTATE_CAUSE cause);

#endif // SipXCall_h__
