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

#ifndef SipXEvents_h__
#define SipXEvents_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/sipXtapi.h"
#include "tapi/sipXtapiEvents.h"
#include "utl/UtlString.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
class SipSession;
class SipMessage;

// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

SIPX_RESULT sipxEventListenerAddInternal(const SIPX_INST hInst,
                                         SIPX_EVENT_CALLBACK_PROC pCallbackProc,
                                         void *pUserData);

SIPX_RESULT sipxEventListenerRemoveInternal(const SIPX_INST hInst, 
                                            SIPX_EVENT_CALLBACK_PROC pCallbackProc, 
                                            void* pUserData);

void sipxFireConfigEvent(const SIPX_INST pInst,                                                        
                         SIPX_CONFIG_EVENT event,
                         void* pEventData);

/**
* Fires a Line Event to the listeners.
*/
void sipxFireLineEvent(SIPX_INST pInst,
                       const UtlString& lineIdentifier,
                       SIPX_LINESTATE_EVENT event,
                       SIPX_LINESTATE_CAUSE cause,
                       int sipResponseCode = 0,
                       const UtlString& sResponseText = NULL);

void sipxFireCallEvent(const SIPX_INST pInst,
                       const UtlString& sCallId,
                       const UtlString& sSessionCallId,
                       const SipSession& pSession,
                       const UtlString& szRemoteAddress,
                       SIPX_CALLSTATE_EVENT event,
                       SIPX_CALLSTATE_CAUSE cause,
                       const void* pEventData = NULL,
                       int sipResponseCode = 0,
                       const UtlString& sResponseText = NULL);

void sipxFireMediaEvent(SIPX_INST pInst,
                        const UtlString& sCallId,
                        const UtlString& sSessionCallId,
                        const UtlString& sRemoteAddress,
                        SIPX_MEDIA_EVENT event,
                        SIPX_MEDIA_CAUSE cause,
                        SIPX_MEDIA_TYPE type,
                        void* pEventData = NULL,
                        void* pCookie = NULL,
                        int playBufferIndex = 0);

/**
* Fires SipPimClient events
*/
void sipxFirePIMEvent(void* userData,
                      const UtlString& fromAddress,
                      const char* textMessage,
                      int textLength,
                      const char* subject,
                      const SipMessage& messageRequest);

/**
* Fires events to interested listener (keepalive events only)
*/
void sipxFireKeepaliveEvent(const SIPX_INST      pInst,                                                        
                            SIPX_KEEPALIVE_EVENT event,
                            SIPX_KEEPALIVE_CAUSE cause,
                            SIPX_KEEPALIVE_TYPE  type,
                            const char*          szRemoteAddress,
                            int                  remotePort,
                            int                  keepAliveSecs,
                            const char*          szFeedbackAddress,
                            int                  feedbackPort);

bool sipxFireSubscriptionStatusEvent(const SIPX_INST pInst,
                                     SIPX_SUBSTATUS_INFO* pInfo);

bool sipxFireNotifyEvent(const SIPX_INST pInst,
                         SIPX_NOTIFY_INFO* pInfo);

bool sipxFireInfoStatusEvent(const SIPX_INST pInst,
                             SIPX_INFO hInfo,
                             SIPX_MESSAGE_STATUS status,
                             int responseCode,
                             const UtlString& sResponseText,
                             SIPX_INFOSTATUS_EVENT event);

bool sipxFireSecurityEvent(const SIPX_INST pInst,
                           const UtlString& sSRTPkey,
                           void* pCertificate,
                           size_t nCertificateSize,
                           SIPX_SECURITY_EVENT event,
                           SIPX_SECURITY_CAUSE cause,
                           const UtlString& sSubjAltName,
                           const UtlString& sSessionCallId,
                           const UtlString& sRemoteAddress);

#endif // SipXEvents_h__
