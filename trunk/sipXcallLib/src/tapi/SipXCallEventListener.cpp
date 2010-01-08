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

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <net/SipDialog.h>
#include <cp/XCpCallManager.h>
#include "tapi/SipXCallEventListener.h"
#include <tapi/CallStateEventMsg.h>
#include <tapi/SipXEventDispatcher.h>
#include "tapi/SipXEvents.h"
#include "tapi/SipXHandleMap.h"
#include <tapi/SipXCore.h>
#include <tapi/SipXCall.h>
#include <tapi/SipXLine.h>
#include <tapi/SipXConference.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
extern SipXHandleMap gCallHandleMap;   // sipXtapiInternal.cpp

// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

SipXCallEventListener::SipXCallEventListener( SIPX_INST pInst )
: OsSharedServerTask("SipXCallEventListener-%d")
, CpCallStateEventListener()
, m_pInst(pInst)
{

}

SipXCallEventListener::~SipXCallEventListener()
{
   release();
}

/* ============================ MANIPULATORS ============================== */

void SipXCallEventListener::OnNewCall( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_NEWCALL, event);
   postMessage(msg);
}

void SipXCallEventListener::OnDialTone( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_DIALTONE, event);
   postMessage(msg);
}

void SipXCallEventListener::OnRemoteOffering( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_REMOTE_OFFERING, event);
   postMessage(msg);
}

void SipXCallEventListener::OnRemoteAlerting( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_REMOTE_ALERTING, event);
   postMessage(msg);
}

void SipXCallEventListener::OnConnected( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_CONNECTED, event);
   postMessage(msg);
}

void SipXCallEventListener::OnBridged( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_BRIDGED, event);
   postMessage(msg);
}

void SipXCallEventListener::OnHeld( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_HELD, event);
   postMessage(msg);
}

void SipXCallEventListener::OnRemoteHeld( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_REMOTE_HELD, event);
   postMessage(msg);
}

void SipXCallEventListener::OnDisconnected( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_DISCONNECTED, event);
   postMessage(msg);
}

void SipXCallEventListener::OnOffering( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_OFFERING, event);
   postMessage(msg);
}

void SipXCallEventListener::OnAlerting( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_ALERTING, event);
   postMessage(msg);
}

void SipXCallEventListener::OnDestroyed( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_DESTROYED, event);
   postMessage(msg);
}

void SipXCallEventListener::OnTransferEvent( const CpCallStateEvent& event )
{
   CallStateEventMsg msg(CALLSTATE_TRANSFER_EVENT, event);
   postMessage(msg);
}

UtlBoolean SipXCallEventListener::handleMessage(OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;

   switch (rRawMsg.getMsgType())
   {
   case CALLSTATEEVENT_MSG:
      {
         CallStateEventMsg* pMsg = dynamic_cast<CallStateEventMsg*>(&rRawMsg);
         if (pMsg)
         {
            // cast succeeded
            const CpCallStateEvent& payload = pMsg->getEventPayloadRef();
            handleCallEvent(payload.m_sCallId, payload.m_pSipDialog,
               pMsg->getEvent(), (SIPX_CALLSTATE_CAUSE)payload.m_cause, payload.m_sOriginalSessionCallId,
               payload.m_sipResponseCode, payload.m_sResponseText, payload.m_sReferredBy, payload.m_sReferTo);
         }
      }
      bResult = TRUE;
      break;
   default:
      break;
   }
   return bResult;
}

void SipXCallEventListener::sipxFireCallEvent(const UtlString& sCallId,
                                              const SipDialog* pSipDialog,
                                              SIPX_CALLSTATE_EVENT event,
                                              SIPX_CALLSTATE_CAUSE cause,
                                              const UtlString& sOriginalSessionCallId /*= NULL*/,
                                              int sipResponseCode /*= 0*/,
                                              const UtlString& sResponseText/* = NULL*/)
{
   CpCallStateEvent payload(sCallId, pSipDialog,
      (CP_CALLSTATE_CAUSE)cause, sOriginalSessionCallId,
      sipResponseCode, sResponseText);
   CallStateEventMsg msg(event, payload);
   postMessage(msg);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

// dont forget to fill szRemoteAddress in call after it is connected
void SipXCallEventListener::handleCallEvent(const UtlString& sCallId, 
                                            const SipDialog* pSipDialog, 
                                            SIPX_CALLSTATE_EVENT event, 
                                            SIPX_CALLSTATE_CAUSE cause, 
                                            const UtlString& sOriginalSessionCallId,
                                            int sipResponseCode,
                                            const UtlString& sResponseText,
                                            const UtlString& sReferredBy,
                                            const UtlString& sReferTo)
{
   UtlString sRemoteAddress;
   UtlString sSessionCallId;
   if (pSipDialog)
   {
      pSipDialog->getRemoteField(sRemoteAddress);
      pSipDialog->getCallId(sSessionCallId);
   }
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxFireCallEvent");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxFireCallEvent CallId=%s RemoteAddress=%s Event=%s:%s",
      sCallId.data(),
      sRemoteAddress.data(),
      convertCallstateEventToString(event),
      convertCallstateCauseToString(cause));

   SIPX_CALL hCall = SIPX_CALL_NULL;
   SIPX_INSTANCE_DATA* pSipXInstance = SAFE_PTR_CAST(SIPX_INSTANCE_DATA, m_pInst);
   assert(pSipXInstance);

   SIPX_CALL_DATA* pCallData = NULL;
   SIPX_LINE hLine = SIPX_LINE_NULL;

   UtlString localField;
   SIPX_CALL hAssociatedCall = SIPX_CALL_NULL;

   if (pSipDialog)
   {
      pSipDialog->getLocalField(localField);
   }

   // If this is an NEW inbound call (first we are hearing of it), then create
   // a call handle/data structure for it.
   if (event == CALLSTATE_NEWCALL || (event == CALLSTATE_DIALTONE && cause == CALLSTATE_CAUSE_TRANSFER))
   {
      pCallData = new SIPX_CALL_DATA();
      pCallData->m_mutex.acquire();
      UtlBoolean res = gCallHandleMap.allocHandle(hCall, pCallData);
      if (!res)
      {
         assert(false);
         // handle allocation failed
         OsSysLog::add(FAC_SIPXTAPI, PRI_ERR, 
            "allocHandle failed to allocate a handle for sCallId=%s", sCallId.data());
         delete pCallData;
         return;
      }
      else
      {
         // new call was successfully allocated

         // Increment call count
         pSipXInstance->lock.acquire();
         pSipXInstance->nCalls++;
         pSipXInstance->lock.release();
      }

      pCallData->m_state = SIPX_INTERNAL_CALLSTATE_UNKNOWN;

      pCallData->m_abstractCallId = sCallId;
      if (pSipDialog)
      {
         pCallData->m_sipDialog = *pSipDialog;
      }
      pCallData->m_pInst = pSipXInstance;
      pCallData->m_mutex.release();

      // VERIFY
      if (!sOriginalSessionCallId.isNull()) // event data during newcall => call transfer original call
      {
         hAssociatedCall = sipxCallLookupHandleBySessionCallId(sOriginalSessionCallId, m_pInst);

         // Make sure we remove the call instead of allowing a drop.  When acting
         // as a transfer target, we are performing surgery on a CpPeerCall.  We
         // want to remove the call leg -- not drop the entire call.

         // VERIFY THIS CODE
         if ((hAssociatedCall) && (cause == CALLSTATE_CAUSE_TRANSFERRED))
         {
            // get the callstate of the replaced leg
            SIPX_CALL_DATA* pOldCallData = sipxCallLookup(hAssociatedCall, SIPX_LOCK_READ, stackLogger);
            UtlBoolean bCallHoldInvoked = FALSE;
            if (pOldCallData)
            {
               bCallHoldInvoked = pOldCallData->m_bCallHoldInvoked;
               sipxCallReleaseLock(pOldCallData, SIPX_LOCK_READ, stackLogger);
            }

            if (bCallHoldInvoked)
            {
               SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);
               if (pData)
               {
                  pData->m_bHoldAfterConnect = true;
                  sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
               }
            }
            sipxCallSetRemoveInsteadofDrop(hAssociatedCall);

            SIPX_CONF hConf = sipxCallGetConf(hAssociatedCall);
            if (hConf)
            {
               sipxAddCallHandleToConf(hCall, hConf);
            }
         }
         // NOT SURE WHETHER THIS IS OK
         else if (hAssociatedCall && (cause == CALLSTATE_CAUSE_TRANSFER))
         {
            // This is the case where we are the transferee -- we want to
            // make sure that the new call is part of the conference
            SIPX_CONF hConf = sipxCallGetConf(hAssociatedCall);
            if (hConf)
            {
               // The original call was part of a transfer -- make sure the
               // replacement leg is also part of the conference.
               sipxAddCallHandleToConf(hCall, hConf);
            }
         }
      }
   }
   else // it is an existing call
   {
      if (event == CALLSTATE_DIALTONE && cause != CALLSTATE_CAUSE_CONFERENCE)
      {
         // for CALLSTATE_DIALTONE, only callId is known, as call is not yet connected
         // except for conference dialtone, where sSessionCallId is known
         hCall = sipxCallLookupHandleByCallId(sCallId, m_pInst);
      }
      else if (event == CALLSTATE_DESTROYED)
      {
         // if call was not connected only sCallId will be valid, therefore if call is
         // not found by sSessionCallId, try sCallId

         hCall = sipxCallLookupHandleBySessionCallId(sSessionCallId, m_pInst);
         if (hCall == SIPX_CALL_NULL)
         {
            // call was not found, probably wasn't connected
            // try lookup by sCallId
            hCall = sipxCallLookupHandleByCallId(sCallId, m_pInst);
         }
      }
      else
      {
         // for usual call state, search by sessionCallId
         hCall = sipxCallLookupHandleBySessionCallId(sSessionCallId, m_pInst);
      }

      // try to update dialog state
      pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);
      if (pCallData)
      {
         // update dialog state
         if (pSipDialog)
         {
            pCallData->m_sipDialog = *pSipDialog;
         }
         sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
      }

      // get some info for call handle to use later
      if (!sipxCallGetCommonData(hCall, &pSipXInstance, NULL, NULL, NULL, &localField))
      {
         OsSysLog::add(FAC_SIPXTAPI, PRI_ERR,
            "event sipXtapiEvents: Unable to find call data for handle: %d, callid=%s, address=%s, M=%s, m=%s\n", 
            hCall, 
            sCallId.data(), 
            sRemoteAddress.data(), 
            convertCallstateEventToString(event), 
            convertCallstateCauseToString(cause));
      }
   }

   // Filter duplicate events
   UtlBoolean bDuplicateEvent = FALSE;
   SIPX_CALLSTATE_EVENT lastEvent;
   SIPX_CALLSTATE_CAUSE lastCause;
   SIPX_INTERNAL_CALLSTATE state = SIPX_INTERNAL_CALLSTATE_UNKNOWN;
   if (sipxCallGetState(hCall, lastEvent, lastCause, state))
   {
      // Filter our duplicate events
      if ((lastEvent == event) && (lastCause == cause))
      {
         bDuplicateEvent = TRUE;
      }          
   }

   if (bDuplicateEvent)
   {
      return;
   }


   // Only proceed if this isn't a duplicate event and we have a valid 
   // call handle.
   if (hCall != SIPX_CALL_NULL)
   {
      // Find Line
      UtlString requestUri; 
      if (pSipDialog)
      {
         if (!pSipDialog->isLocalInitiatedDialog())
         {
            // inbound call
            Url localRequestUri;
            pSipDialog->getLocalRequestUri(localRequestUri);
            localRequestUri.toString(requestUri);
         } // for outbound calls request uri of inbound requests is unknown
      }

      hLine = sipxLineLookupHandle(pSipXInstance, localField.data(), requestUri.data());
      if (hLine == SIPX_LINE_NULL)
      {
         // no line exists for the lineId
         // log it
         OsSysLog::add(FAC_SIPXTAPI, PRI_NOTICE, "unknown line id = %s\n", localField.data());
      }

      SIPX_CALLSTATE_INFO callInfo;
      memset((void*) &callInfo, 0, sizeof(SIPX_CALLSTATE_INFO));

      callInfo.event = event;
      callInfo.cause = cause;
      callInfo.hCall = hCall;
      callInfo.hLine = hLine;
      callInfo.hAssociatedCall = hAssociatedCall;
      callInfo.nSize = sizeof(SIPX_CALLSTATE_INFO);
      callInfo.sipResponseCode = sipResponseCode;
      callInfo.szSipResponseText = sResponseText.data(); // safe to do as SipXEventDispatcher makes a copy
      callInfo.szReferredBy = sReferredBy.data(); // safe to do as SipXEventDispatcher makes a copy
      callInfo.szReferTo = sReferTo.data(); // safe to do as SipXEventDispatcher makes a copy

      // store call state
      sipxCallSetState(hCall, event, cause);

      // fire event
      SipXEventDispatcher::dispatchEvent(m_pInst, EVENT_CATEGORY_CALLSTATE, &callInfo);

      // If this is a DESTROY message, free up resources
      if (CALLSTATE_DESTROYED == event)
      {
         SIPX_CONF hConf = sipxCallGetConf(hCall);
         if (hConf != SIPX_CONF_NULL)
         {
            // remove call from conference
            sipxRemoveCallHandleFromConf(hConf, hCall);
         }
         // free call object
         sipxCallObjectFree(hCall, stackLogger);
      }
   }

   // if call is disconnected and is in conference, or should be just removed not dropped
   if (CALLSTATE_DISCONNECTED == event && 
      (sipxCallGetConf(hCall) != SIPX_CONF_NULL || sipxCallIsRemoveInsteadOfDropSet(hCall)))
   {
      // If a leg of a conference is destroyed, simulate the audio stop and 
      // call destroyed events.

      SIPX_CALL_DATA* pCallData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);
      if (pCallData)
      {
         pCallData->m_pInst->pCallManager->dropConferenceConnection(pCallData->m_abstractCallId, pCallData->m_sipDialog);
         sipxCallReleaseLock(pCallData, SIPX_LOCK_READ, stackLogger);
      }

      // fire simulated local media events
      if (pCallData->m_lastLocalMediaAudioEvent == MEDIA_LOCAL_START)
      {
         pCallData->m_pInst->pMediaEventListener->sipxFireMediaEvent(sCallId, sSessionCallId, sRemoteAddress, 
            MEDIA_LOCAL_STOP, MEDIA_CAUSE_NORMAL, MEDIA_TYPE_AUDIO);
      }

      if (pCallData->m_lastLocalMediaVideoEvent == MEDIA_LOCAL_START)
      {
         pCallData->m_pInst->pMediaEventListener->sipxFireMediaEvent(sCallId,  sSessionCallId,sRemoteAddress, 
            MEDIA_LOCAL_STOP, MEDIA_CAUSE_NORMAL, MEDIA_TYPE_VIDEO);
      }

      // fire simulated remote media events
      if ((pCallData->m_lastRemoteMediaAudioEvent == MEDIA_REMOTE_START) || 
         (pCallData->m_lastRemoteMediaAudioEvent == MEDIA_REMOTE_SILENT) ||
         (pCallData->m_lastRemoteMediaAudioEvent == MEDIA_REMOTE_ACTIVE))
      {
         pCallData->m_pInst->pMediaEventListener->sipxFireMediaEvent(sCallId,  sSessionCallId,sRemoteAddress, 
            MEDIA_REMOTE_STOP, MEDIA_CAUSE_NORMAL, MEDIA_TYPE_AUDIO);
      }

      if ((pCallData->m_lastRemoteMediaVideoEvent == MEDIA_REMOTE_START) || 
         (pCallData->m_lastRemoteMediaVideoEvent == MEDIA_REMOTE_SILENT) ||
         (pCallData->m_lastRemoteMediaVideoEvent == MEDIA_REMOTE_ACTIVE))
      {
         pCallData->m_pInst->pMediaEventListener->sipxFireMediaEvent(sCallId, sSessionCallId, sRemoteAddress, 
            MEDIA_REMOTE_STOP, MEDIA_CAUSE_NORMAL, MEDIA_TYPE_VIDEO);
      }

      // also fire destroyed event asynchronously
      sipxFireCallEvent(sCallId, pSipDialog,
         CALLSTATE_DESTROYED,
         CALLSTATE_CAUSE_NORMAL,
         sOriginalSessionCallId);
   }

   // check for the bHoldAfterConnect flag.  If it is true, start a hold
   if (CALLSTATE_CONNECTED == event || CALLSTATE_HELD == event)
   {
      SIPX_CALL_DATA* pCallData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);
      if (pCallData)
      {
         bool bHoldAfterConnect = pCallData->m_bHoldAfterConnect;
         sipxCallReleaseLock(pCallData, SIPX_LOCK_READ, stackLogger);

         if (bHoldAfterConnect)
         {
            // release lock before acquiring global lock
            sipxCallHold(hCall);

            // now update call
            pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);
            if (pCallData)
            {
               pCallData->m_bHoldAfterConnect = false;
               sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
            }
         }
      }
   }
}

/* ============================ FUNCTIONS ================================= */

