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
#include <net/SipDialog.h>
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
   UtlString m_callId; ///< Id identifying CpPeerCall instance
   UtlString m_sessionCallId; ///< SIP CallId used in SIP Messages, identifies SipConnection
   UtlString m_ghostCallId;
   UtlString m_remoteAddress;
   UtlString m_fromUrl; ///< from URL used for call, will contain tag. For outbound calls tag is added later.
   Url m_fullLineUrl; ///< like lineURI, but with display name, field parameters or brackets
   Url m_lineUri; ///< URI of line. Copy of m_lineURI from SIPX_LINE_DATA. This one will never contain a tag.
   UtlString m_remoteContactAddress;///< Remote Contact URI
   SipDialog m_sipDialog; ///< sip dialog of this call
   SIPX_LINE m_hLine;
   SIPX_INSTANCE_DATA* m_pInst;
   OsMutex m_mutex;
   SIPX_CONF m_hConf;
   SIPX_SECURITY_ATTRIBUTES m_security;
   SIPX_VIDEO_DISPLAY m_display;
   UtlBoolean m_bRemoveInsteadOfDrop;   /** Remove the call instead of dropping it 
                                       -- this is used as part of consultative 
                                       transfer when we are the transfer target 
                                       and need to replace a call leg within 
                                       the same CpPeerCall. */
   SIPX_CALLSTATE_EVENT m_lastCallstateEvent;
   SIPX_CALLSTATE_CAUSE m_lastCallstateCause;

   SIPX_MEDIA_EVENT m_lastLocalMediaAudioEvent;
   SIPX_MEDIA_EVENT m_lastLocalMediaVideoEvent;
   SIPX_MEDIA_EVENT m_lastRemoteMediaAudioEvent;
   SIPX_MEDIA_EVENT m_lastRemoteMediaVideoEvent;

   SIPX_INTERNAL_CALLSTATE m_state;
   UtlBoolean m_bInFocus;
   int m_connectionId;                  /** Cache the connection id */
   SIPX_TRANSPORT m_hTransport;
   bool m_bHoldAfterConnect;            /** Used if we are the transfer target, and the
                                      replaced call is HELD or REMOTE_HELD, then
                                      this flag is set, and indicates that the call
                                      should be placed on hold after the connection
                                      is established. */
   UtlBoolean m_bCallHoldInvoked;             /** Set to true if sipxCallHold has been invoked.
                                      Set to fales if sipxCallUnhold has been invoked. */                                          
   SIPX_CALL_DATA()
      : m_mutex(OsMutex::Q_FIFO),
      m_callId(NULL),
      m_sessionCallId(NULL),
      m_ghostCallId(NULL),
      m_remoteAddress(NULL),
      m_fromUrl(NULL),
      m_lineUri(),
      m_remoteContactAddress(NULL),
      m_sipDialog(NULL),
      m_hLine(0),
      m_pInst(NULL),
      m_hConf(NULL),
      m_security(),
      m_display(),
      m_bRemoveInsteadOfDrop(false),
      m_lastCallstateEvent(CALLSTATE_UNKNOWN),
      m_lastCallstateCause(CALLSTATE_CAUSE_UNKNOWN),
      m_lastLocalMediaAudioEvent(MEDIA_UNKNOWN),
      m_lastLocalMediaVideoEvent(MEDIA_UNKNOWN),
      m_lastRemoteMediaAudioEvent(MEDIA_UNKNOWN),
      m_lastRemoteMediaVideoEvent(MEDIA_UNKNOWN),
      m_state(SIPX_INTERNAL_CALLSTATE_UNKNOWN),
      m_bInFocus(false),
      m_connectionId(0),
      m_hTransport(0),
      m_bHoldAfterConnect(false),
      m_bCallHoldInvoked(false)
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
* Get the list of active calls for the specified sipxtapi instance.
*/
SIPXTAPI_API SIPX_RESULT sipxGetAllAbstractCallIds(SIPX_INST hInst, UtlSList& idList);

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
                                 bool bFireDialtone = true,
                                 bool bIsConferenceCall = false);


SIPX_RESULT sipxCallDrop(SIPX_CALL& hCall);

UtlBoolean sipxCallSetState(SIPX_CALL hCall, 
                            SIPX_CALLSTATE_EVENT event,
                            SIPX_CALLSTATE_CAUSE cause);

#endif // SipXCall_h__
