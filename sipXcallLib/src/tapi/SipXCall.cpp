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
#include <assert.h>

// APPLICATION INCLUDES
#include <utl/UtlInit.h>

#include <utl/UtlVoidPtr.h>
#include <utl/UtlHashMapIterator.h>
#include <net/SipUserAgent.h>
#include <net/SipLine.h>
#include "tapi/SipXHandleMap.h"
#include "tapi/SipXCall.h"
#include "tapi/SipXLine.h"
#include "tapi/SecurityHelper.h"
#include "tapi/SipXCallEventListener.h"
#include "cp/XCpCallManager.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
SipXHandleMap gCallHandleMap(1, SIPX_CALL_NULL);  /**< Global Map of call handles */

// GLOBAL FUNCTIONS

/* ============================ FUNCTIONS ================================= */


UtlBoolean sipxCallSetState(SIPX_CALL hCall,
                            SIPX_CALLSTATE_EVENT event,
                            SIPX_CALLSTATE_CAUSE cause)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallSetState");

   UtlBoolean bSuccess = false;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, logItem);

   if (pData)
   {
      // Store state
      pData->m_lastCallstateEvent = event;
      pData->m_lastCallstateCause = cause;

      // Calculate internal state
      switch (event)
      {
      case CALLSTATE_NEWCALL:
         break;
      case CALLSTATE_DIALTONE:
      case CALLSTATE_REMOTE_OFFERING:
      case CALLSTATE_REMOTE_ALERTING:
         pData->m_state = SIPX_INTERNAL_CALLSTATE_OUTBOUND_ATTEMPT;
         break;
      case CALLSTATE_HELD:
         pData->m_state = SIPX_INTERNAL_CALLSTATE_HELD;
         pData->m_bInFocus = false;
         break;
      case CALLSTATE_REMOTE_HELD:
         pData->m_state = SIPX_INTERNAL_CALLSTATE_REMOTE_HELD;
         pData->m_bInFocus = false;
         break;
      case CALLSTATE_BRIDGED:
         pData->m_state = SIPX_INTERNAL_CALLSTATE_BRIDGED;
         pData->m_bInFocus = false; // ????
         break;
      case CALLSTATE_CONNECTED:
         switch (cause)
         {
         case CALLSTATE_CAUSE_NORMAL:
            pData->m_state = SIPX_INTERNAL_CALLSTATE_CONNECTED;
            pData->m_bInFocus = true;
            break;
         case CALLSTATE_CAUSE_REQUEST_NOT_ACCEPTED:
            pData->m_state = SIPX_INTERNAL_CALLSTATE_CONNECTED;
            break;
         default:
            break;
         }
         break;
      case CALLSTATE_DISCONNECTED:
         pData->m_state = SIPX_INTERNAL_CALLSTATE_DISCONNECTED;
         pData->m_bInFocus = false;
         break;
      case CALLSTATE_OFFERING:
      case CALLSTATE_ALERTING:
         pData->m_state = SIPX_INTERNAL_CALLSTATE_OUTBOUND_ATTEMPT;
         break;
      case CALLSTATE_DESTROYED:
         pData->m_bInFocus = false;
         break;
      case CALLSTATE_TRANSFER_EVENT:
         break;
      default:
         break;
      }

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, logItem);
   }

   return bSuccess;
}


SIPX_CALL sipxCallLookupHandleBySessionCallId( const UtlString& sessionCallID, SIPX_INST pInst )
{
   SIPX_CALL hCall = 0;
   SIPX_CALL_DATA* pData = NULL;
   OsStatus status;

   gCallHandleMap.lock();
   // control iterator scope
   {
      UtlHashMapIterator iter(gCallHandleMap);

      UtlInt* pIndex = NULL;
      UtlVoidPtr* pObj = NULL;

      while ((pIndex = dynamic_cast<UtlInt*>(iter())))       
      {
         pObj = dynamic_cast<UtlVoidPtr*>(gCallHandleMap.findValue(pIndex));

         assert(pObj); // if it's NULL, then it's a bug
         if (pObj)
         {
            pData = (SIPX_CALL_DATA*)pObj->getValue();
            assert(pData);

            if (pData)
            {
               status = pData->m_mutex.acquire();
               assert(status == OS_SUCCESS);

               if (pData->m_sessionCallId.compareTo(sessionCallID) == 0 && 
                   pData->m_pInst == pInst)
               {
                  hCall = pIndex->getValue();
                  status = pData->m_mutex.release();
                  assert(status == OS_SUCCESS);
                  break;
               }

               status = pData->m_mutex.release();
               assert(status == OS_SUCCESS);
            }            
         }
      }
   }
   gCallHandleMap.unlock();

   return hCall;
}


SIPX_CALL sipxCallLookupHandleByCallId( const UtlString& callID, SIPX_INST pInst )
{
   SIPX_CALL hCall = 0;
   SIPX_CALL_DATA* pData = NULL;
   OsStatus status;

   gCallHandleMap.lock();
   // control iterator scope
   {
      UtlHashMapIterator iter(gCallHandleMap);

      UtlInt* pIndex = NULL;
      UtlVoidPtr* pObj = NULL;

      while ((pIndex = dynamic_cast<UtlInt*>(iter())))       
      {
         pObj = dynamic_cast<UtlVoidPtr*>(gCallHandleMap.findValue(pIndex));

         assert(pObj); // if it's NULL, then it's a bug
         if (pObj)
         {
            pData = (SIPX_CALL_DATA*)pObj->getValue();
            assert(pData);

            if (pData)
            {
               status = pData->m_mutex.acquire();
               assert(status == OS_SUCCESS);

               if (pData->m_callId.compareTo(callID) == 0 && 
                  pData->m_pInst == pInst)
               {
                  hCall = pIndex->getValue();
                  status = pData->m_mutex.release();
                  assert(status == OS_SUCCESS);
                  break;
               }

               status = pData->m_mutex.release();
               assert(status == OS_SUCCESS);
            }            
         }
      }
   }
   gCallHandleMap.unlock();

   return hCall;
}

/**
* Frees call object.
*/

void sipxCallObjectFree(const SIPX_CALL hCall, const OsStackTraceLogger& oneBackInStack)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallObjectFree", oneBackInStack);

   gCallHandleMap.lock();
   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, logItem);

   if (pData)
   {
      const void* pRC = gCallHandleMap.removeHandle(hCall); 
      gCallHandleMap.unlock(); // we can release lock now
      assert(pRC); // if NULL, then something is bad :(

      pData->m_pInst->lock.acquire();
      pData->m_pInst->nCalls--;
      assert(pData->m_pInst->nCalls >= 0);
      pData->m_pInst->lock.release();

      destroyCallData(pData);
   }
   else
   {
      gCallHandleMap.unlock(); // we can release lock now
   }
}

/**
* Finds call by call handle and acquires lock on it.
* OsMutex is used internally instead of OsRWMutex to allow recursive locking.
*/

SIPX_CALL_DATA* sipxCallLookup(const SIPX_CALL hCall,
                               SIPX_LOCK_TYPE type,
                               const OsStackTraceLogger& oneBackInStack)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallLookup", oneBackInStack);
   SIPX_CALL_DATA* pRC = NULL;
   OsStatus status = OS_FAILED;

   gCallHandleMap.lock();
   pRC = (SIPX_CALL_DATA*)gCallHandleMap.findHandle(hCall);

   if (pRC)
   {
      if (validCallData(pRC))
      {
         switch (type)
         {
         case SIPX_LOCK_READ:
            status = pRC->m_mutex.acquire();
            assert(status == OS_SUCCESS);
            break;
         case SIPX_LOCK_WRITE:
            status = pRC->m_mutex.acquire();
            assert(status == OS_SUCCESS);
            break;
         default:
            break;
         }
      }
      else // call was found but call data is not completely valid
      {
         // fire assert, something is not right, fix it
         assert(false); // this only works in debug mode
         pRC = NULL; // we only get here in release mode
      }
   }
   gCallHandleMap.unlock();

   return pRC;
}

/**
* Checks whether call data is valid.
*/

UtlBoolean validCallData(SIPX_CALL_DATA* pData)
{
   return (pData &&
      !pData->m_callId.isNull() && 
      !pData->m_fromUrl.isNull() && 
      pData->m_pInst &&
      pData->m_pInst->pCallManager && 
      pData->m_pInst->pRefreshManager &&
      pData->m_pInst->pLineManager);
}

/**
* Releases call lock.
*/

void sipxCallReleaseLock(SIPX_CALL_DATA* pData,
                         SIPX_LOCK_TYPE type,
                         const OsStackTraceLogger& oneBackInStack)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallReleaseLock", oneBackInStack);
   OsStatus status;

   if (type != SIPX_LOCK_NONE)
   {
      if (validCallData(pData))
      {
         switch (type)
         {
         case SIPX_LOCK_READ:
            status = pData->m_mutex.release();
            assert(status == OS_SUCCESS);
            break;
         case SIPX_LOCK_WRITE:
            status = pData->m_mutex.release();
            assert(status == OS_SUCCESS);
            break;
         default:
            break;
         }
      }
      else
      {
         // something is bad if call data is not valid, fix bug
         assert(false);
      }
   }   
}

void destroyCallData(SIPX_CALL_DATA* pData)
{
   if (pData)
   {
      // no need to release mutex, nobody should be waiting on it or its a bug
      delete pData;
   }
}


UtlBoolean sipxCallGetMediaState(SIPX_CALL         hCall,
                                 SIPX_MEDIA_EVENT& lastLocalMediaAudioEvent,
                                 SIPX_MEDIA_EVENT& lastLocalMediaVideoEvent,
                                 SIPX_MEDIA_EVENT& lastRemoteMediaAudioEvent,
                                 SIPX_MEDIA_EVENT& lastRemoteMediaVideoEvent)
{
   UtlBoolean bSuccess = FALSE;
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetMediaState");

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, logItem);

   if (pData)
   {
      lastLocalMediaAudioEvent = pData->m_lastLocalMediaAudioEvent;
      lastLocalMediaVideoEvent = pData->m_lastLocalMediaVideoEvent;
      lastRemoteMediaAudioEvent = pData->m_lastRemoteMediaAudioEvent;
      lastRemoteMediaVideoEvent = pData->m_lastRemoteMediaVideoEvent;

      bSuccess = TRUE;

      sipxCallReleaseLock(pData, SIPX_LOCK_READ, logItem);        
   }

   return bSuccess;
}


UtlBoolean sipxCallSetMediaState(SIPX_CALL hCall,
                                 SIPX_MEDIA_EVENT event,
                                 SIPX_MEDIA_TYPE type) 
{
   UtlBoolean bSuccess = FALSE;

   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallSetMediaState");

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, logItem);
   if (pData)
   {
      switch (event)
      {
      case MEDIA_LOCAL_START:
      case MEDIA_LOCAL_STOP:
         switch (type)
         {
         case MEDIA_TYPE_AUDIO:
            pData->m_lastLocalMediaAudioEvent = event;
            bSuccess = TRUE;
            break;
         case MEDIA_TYPE_VIDEO:
            pData->m_lastLocalMediaVideoEvent = event;
            bSuccess = TRUE;
            break;
         default:
            break;
         }
         break;
      case MEDIA_REMOTE_START:
      case MEDIA_REMOTE_STOP:
      case MEDIA_REMOTE_SILENT:
         switch (type)
         {
         case MEDIA_TYPE_AUDIO:
            pData->m_lastRemoteMediaAudioEvent = event;
            bSuccess = TRUE;
            break;
         case MEDIA_TYPE_VIDEO:
            pData->m_lastRemoteMediaVideoEvent = event;
            bSuccess = TRUE;
            break;
         default:
            break;
         }
         break;
      default:
         break;
      }

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, logItem);        
   }

   return bSuccess;
}


UtlBoolean sipxCallGetState(SIPX_CALL hCall, 
                            SIPX_CALLSTATE_EVENT& lastEvent,
                            SIPX_CALLSTATE_CAUSE& lastCause,
                            SIPX_INTERNAL_CALLSTATE& state) 
{
   UtlBoolean bSuccess = FALSE;
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetState");

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, logItem);
   if (pData)
   {
      lastEvent = pData->m_lastCallstateEvent;
      lastCause = pData->m_lastCallstateCause;
      state = pData->m_state;
      bSuccess = TRUE;
      sipxCallReleaseLock(pData, SIPX_LOCK_READ, logItem);
   }

   return bSuccess;
}


UtlBoolean sipxCallGetCommonData(SIPX_CALL hCall,
                                 SIPX_INSTANCE_DATA** pInst,
                                 UtlString* pCallId,
                                 UtlString* pSessionCallId,
                                 UtlString* pStrRemoteAddress,
                                 UtlString* pFromUrl,
                                 UtlString* pGhostCallId, 
                                 UtlString* pRemoteContactAddress) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetCommonData");

   UtlBoolean bSuccess = FALSE;
   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, logItem);

   if (pData)
   {
      if (pInst)
      {
         *pInst = pData->m_pInst;
      }

      if (pCallId)
      {
          *pCallId = pData->m_callId;
      }

      if (pSessionCallId)
      {
         *pSessionCallId = pData->m_sessionCallId;
      }

      if (pStrRemoteAddress)
      {
         *pStrRemoteAddress = pData->m_remoteAddress;
      }

      if (pFromUrl)
      {
         *pFromUrl = pData->m_fromUrl;
      }

      if (pGhostCallId)
      {
         *pGhostCallId = pData->m_ghostCallId;
      }

      if (pRemoteContactAddress)
      {
         *pRemoteContactAddress = pData->m_remoteContactAddress;
      }
      bSuccess = TRUE;

      sipxCallReleaseLock(pData, SIPX_LOCK_READ, logItem);
   }

   return bSuccess;
}


// returns ids of XCpCall and XCpConference
SIPXTAPI_API SIPX_RESULT sipxGetAllAbstractCallIds(SIPX_INST hInst, UtlSList& idList)
{
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*) hInst;
   if (pInst)
   {
      OsStatus status = pInst->pCallManager->getAbstractCallIds(idList);
      if (status == OS_SUCCESS)
      {
         rc = SIPX_RESULT_SUCCESS;
      }
   }
   else
   {
      rc = SIPX_RESULT_INVALID_ARGS;
   }

   return rc;
}


// Destroy all calls, no need to send simulated DESTROYED events, as the way
// we shutdown OsServerTasks is going to change (optionally they will wait until all messages
// are processed)
void sipxCallDestroyAll(const SIPX_INST hInst) 
{
   SIPX_CALL hCall = SIPX_CALL_NULL;        
   UtlSList sessionCallIdList;
   UtlString* pSessionCallId;

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*) hInst;
   if (pInst)
   {
      sipxGetAllAbstractCallIds(hInst, sessionCallIdList);

      // iterate over all session call ids
      UtlSListIterator itor(sessionCallIdList);
      while(pSessionCallId = dynamic_cast<UtlString*>(itor()))
      {
         hCall = sipxCallLookupHandleBySessionCallId(*pSessionCallId, pInst);

         if (hCall)
         {
            sipxCallDestroy(&hCall);
         }
      }

      sessionCallIdList.destroyAll();
   }
}


SIPX_CONF sipxCallGetConf(SIPX_CALL hCall) 
{
   SIPX_CONF hConf = SIPX_CONF_NULL;
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetConf");

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, logItem);
   if (pData)
   {
      hConf = pData->m_hConf;
      sipxCallReleaseLock(pData, SIPX_LOCK_READ, logItem);
   }

   return hConf;
}


SIPX_CONTACT_TYPE sipxCallGetLineContactType(SIPX_CALL hCall) 
{
   SIPX_CONTACT_TYPE contactType = CONTACT_AUTO;
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetLineContactType");

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, logItem);
   if (pData)
   {
      SIPX_LINE hLine = pData->m_hLine;
      sipxCallReleaseLock(pData, SIPX_LOCK_READ, logItem);

      SIPX_LINE_DATA* pLineData = sipxLineLookup(hLine, SIPX_LOCK_READ, logItem);

      if (pLineData)
      {
         contactType = pLineData->m_contactType;
         sipxLineReleaseLock(pLineData, SIPX_LOCK_READ, logItem);
      }
   }

   return contactType;
}


UtlBoolean sipxCallSetRemoveInsteadofDrop(SIPX_CALL hCall) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallSetRemoveInsteadOfDrop");
   UtlBoolean bSuccess = FALSE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, logItem);
   if (pData)
   {
      pData->m_bRemoveInsteadOfDrop = TRUE;
      bSuccess = TRUE;

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, logItem);
   }

   return bSuccess;
}


UtlBoolean sipxCallIsRemoveInsteadOfDropSet(SIPX_CALL hCall)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallIsREmoveInsteadOfDropSet");
   UtlBoolean bShouldRemove = FALSE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, logItem);
   if (pData)
   {
      bShouldRemove = pData->m_bRemoveInsteadOfDrop;

      sipxCallReleaseLock(pData, SIPX_LOCK_READ, logItem);
   }

   return bShouldRemove;
}


// this function assumes call is not in conference
SIPX_RESULT sipxCallDrop(SIPX_CALL& hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallDrop");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallDrop hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pData)
   {
      assert(pData->m_hConf == SIPX_CONF_NULL);
      if (pData->m_state != SIPX_INTERNAL_CALLSTATE_DESTROYING)
      {
         pData->m_state = SIPX_INTERNAL_CALLSTATE_DESTROYING;
         pData->m_pInst->pCallManager->dropAbstractCallConnection(pData->m_callId, pData->m_sipDialog);

         hCall = SIPX_CALL_NULL;
         sr = SIPX_RESULT_SUCCESS;
      }

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
   }

   return sr;
}


// either hLine or szLine must be available
SIPX_RESULT sipxCallCreateHelper(const SIPX_INST hInst,
                                 const SIPX_LINE hLine,
                                 const char* szLine,
                                 const SIPX_CONF hConf,
                                 SIPX_CALL* phCall,
                                 const UtlString& sAbstractCallId,
                                 const UtlString& sSipCallId,
                                 bool bFireDialtone,
                                 bool bIsConferenceCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallCreateHelper");
   OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG, 
      "sipxCallCreateHelper, hInst=%i, hLine=%i, hConf=%i",
      hInst, hLine, hConf);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      if (phCall)
      {
         // set line URI either by handle, or string
         UtlString sLineUri;
         UtlString sFullLineUrl;
         if (hLine != SIPX_LINE_NULL)
         {
            SIPX_LINE_DATA* pLine = sipxLineLookup(hLine, SIPX_LOCK_READ, stackLogger);
            if (pLine)
            {
               sLineUri = pLine->m_lineUri.toString();
               sFullLineUrl = pLine->m_fullLineUrl.toString();
               sipxLineReleaseLock(pLine, SIPX_LOCK_READ, stackLogger);
            }
         }
         else
         {
            sLineUri = SipLine::getLineUri(UtlString(szLine)).toString();// unique line id, sip uri
            sFullLineUrl = SipLine::getFullLineUrl(UtlString(szLine)).toString();// used in from field, sip url
         }
         
         // we now must have a valid line URI - should be not null
         if (!sLineUri.isNull())
         {
            Url lineUri(sLineUri); // reconstruct lineURI Url object from string
            SIPX_CALL_DATA* pData = new SIPX_CALL_DATA();
            // Set line URI in call
            pData->m_fullLineUrl = sFullLineUrl;
            pData->m_fromUrl = sFullLineUrl; // keep the future fromURI, which will also get a tag
            pData->m_lineUri = lineUri; // keep also original lineURI, which never gets a tag
            pData->m_mutex.acquire();
            UtlBoolean res = gCallHandleMap.allocHandle(*phCall, pData);

            if (res)
            {
               // Increment call count
               pInst->lock.acquire();
               pInst->nCalls++;
               pInst->lock.release();

               if (sAbstractCallId.isNull())
               {
                  if (!bIsConferenceCall)
                  {
                     // we don't want to select callId, generate one
                     pData->m_callId = pInst->pCallManager->getNewCallId();
                  }
                  else
                  {
                     pData->m_callId = pInst->pCallManager->getNewConferenceId();
                  }
               }
               else
               {
                  // we want custom callId, use it
                  pData->m_callId = sAbstractCallId;
               }
               
               // Set Conference handle
               pData->m_hConf = hConf;

               // Set Line handle
               pData->m_hLine = hLine;

               // Remote Address
               pData->m_remoteAddress = NULL;

               // Connection Id, initialize to -1, cache later
               pData->m_connectionId = -1;

               // Store Instance
               pData->m_pInst = pInst;

               // sessionCallId will be set when call is connected, but for conference
               // we set it here, so that event system can find this call for dialtone event
               pData->m_sessionCallId = sSipCallId;

               if (!bIsConferenceCall)
               {
                  // only create real call if it is requested
                  // for conference add, we don't create it here
                  pInst->pCallManager->createCall(pData->m_callId);
               }

               if (bFireDialtone)
               {
                  UtlString tmpSessionCallId = pData->m_sessionCallId;
                  UtlString tmpCallId = pData->m_callId;
                  pData->m_mutex.release();

                  // fire dialtone event manually
                  pInst->pCallEventListener->OnDialTone(CpCallStateEvent(tmpSessionCallId,
                           tmpCallId,
                           SipSession(),
                           NULL,
                           bIsConferenceCall ? CALLSTATE_CAUSE_CONFERENCE : CALLSTATE_CAUSE_NORMAL));
               }
               else
               {
                  pData->m_mutex.release();
               }               

               sr = SIPX_RESULT_SUCCESS;
            }
            else
            {
               // handle allocation failure
               OsSysLog::add(FAC_SIPXTAPI, PRI_ERR, 
                  "allocHandle failed to allocate a handle");
               // don't release mutex, no need for it
               delete pData;
            }
         }
      }
      else
      {
         sr = SIPX_RESULT_INVALID_ARGS;
      }
   }

   return sr;
}


/*********************************************************************/
/*       Public call handling functions                              */
/*********************************************************************/


SIPXTAPI_API SIPX_RESULT sipxCallCreate(const SIPX_INST hInst,
                                        const SIPX_LINE hLine,
                                        SIPX_CALL* phCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallCreate");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallCreate hInst=%p hLine=%d phCall=%p",
      hInst, hLine, phCall);
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   if (phCall)
   {
      rc = sipxCallCreateHelper(hInst, hLine, NULL, SIPX_CONF_NULL, phCall);
   }
   
   return rc;
}


SIPXTAPI_API SIPX_RESULT sipxCallCreateOnVirtualLine(const SIPX_INST hInst,
                                                     const char* szLine,
                                                     SIPX_CALL* phCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallCreateOnVirtualLine");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallCreateOnVirtualLine hInst=%p szLine=%s phCall=%p",
      hInst, szLine, phCall);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   if (phCall && szLine)
   {
      rc = sipxCallCreateHelper(hInst, SIPX_LINE_NULL, szLine, SIPX_CONF_NULL, phCall);
   }

   return rc;
}


SIPXTAPI_API SIPX_RESULT sipxCallReject(const SIPX_CALL hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallReject");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallReject hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA *pCallData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);
   if (pCallData)
   {   
      if (!pCallData->m_remoteAddress.isNull() && !pCallData->m_sessionCallId.isNull())
      {
         // call has been connected
         pCallData->m_pInst->pCallManager->rejectCallConnection(pCallData->m_callId);
         sr = SIPX_RESULT_SUCCESS;
      }

      sipxCallReleaseLock(pCallData, SIPX_LOCK_READ, stackLogger);
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallRedirect(const SIPX_CALL hCall, const char* szForwardURL)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallRedirect");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallRedirect hCall=%d forwardURL=%s",
      hCall, szForwardURL);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA *pCallData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);
   if (pCallData)
   {
      if (!pCallData->m_remoteAddress.isNull() && !pCallData->m_sessionCallId.isNull() && szForwardURL)
      {
         pCallData->m_pInst->pCallManager->redirectCallConnection(pCallData->m_callId, szForwardURL);
         sr = SIPX_RESULT_SUCCESS;
      }

      sipxCallReleaseLock(pCallData, SIPX_LOCK_READ, stackLogger);
   }
   return sr;
}

SIPXTAPI_API SIPX_RESULT sipxCallAccept(const SIPX_CALL hCall,
                                        SIPX_VIDEO_DISPLAY* const pDisplay,
                                        SIPX_SECURITY_ATTRIBUTES* const pSecurity,
                                        SIPX_CALL_OPTIONS* options,
                                        int bSendEarlyMedia)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAccept");
   UtlBoolean bEnableLocationHeader = FALSE;
   int bandWidth = AUDIO_CODEC_BW_DEFAULT;
   SIPX_CONTACT_ID contactId = 0;
   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   char* pLocationHeader = NULL;

   if (options)
   {
      if (options->cbSize == sizeof(SIPX_CALL_OPTIONS))
      {
         bEnableLocationHeader = options->sendLocation;
         bandWidth = options->bandwidthId;
         contactId = options->contactId;
      }
      else
      {
         // size mismatch
         assert(false);
         return SIPX_RESULT_INVALID_ARGS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallAccept hCall=%d display=%p bEnableLocationHeader=%d bandWidth=%d, contactId=%d bSendEarlyMedia=%s",
      hCall, pDisplay, bEnableLocationHeader, bandWidth, contactId, 
      bSendEarlyMedia ? "true" : "false");

   if (bSendEarlyMedia)
   {
      // not supported
      sr = SIPX_RESULT_NOT_IMPLEMENTED;
      return sr;
   }

   if (contactId < 0)
   {
      // wrong contactID
      return SIPX_RESULT_INVALID_ARGS;
   }

   SIPX_CALL_DATA *pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);
   if (pCallData)
   {
      // check whether the call has a sessionId - then it's real
      if (!pCallData->m_remoteAddress.isNull() && !pCallData->m_sessionCallId.isNull())
      {
         // set up security info
         if (pSecurity)
         {
            // don't generate a random key, and remove one if one it is there
            // (the caller will provide the agreed upon key)
            pSecurity->setSrtpKey("", 30);
         }

         // set the display object
         if (pDisplay)
         {
            pCallData->m_display = *pDisplay;   
         }
         if (pSecurity)
         {
            pCallData->m_security = *pSecurity;
         }

         // setup location header
         pLocationHeader = (bEnableLocationHeader) ? pCallData->m_pInst->szLocationHeader : NULL;
         void* pSecurityVoidPtr = NULL;

         if (pSecurity && pSecurity->getSecurityLevel() > 0)
         {
            pSecurityVoidPtr = (void*)&pCallData->m_security;
         }

         // just posts message
         pCallData->m_pInst->pCallManager->acceptCallConnection(pCallData->m_callId,
            pLocationHeader,
            contactId);

         sr = SIPX_RESULT_SUCCESS;
      }

      sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
   }// else call not found

   return sr;
}

// for conference call bStopRemoteAudio is ignored as only remote audio is stopped
// that is to prevent focus loss from the conference.
SIPXTAPI_API SIPX_RESULT sipxCallHold(const SIPX_CALL hCall,
                                      int  bStopRemoteAudio)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallHold");
   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallHold hCall=%d bStopRemoteAudio=%d",
      hCall, bStopRemoteAudio);

   SIPX_CALL_DATA* pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pCallData)
   {
      if (pCallData->m_state != SIPX_INTERNAL_CALLSTATE_HELD || pCallData->m_bHoldAfterConnect)
      {
         if (pCallData->m_hConf == SIPX_CONF_NULL && bStopRemoteAudio)
         {
            // call is not part of conference
            pCallData->m_pInst->pCallManager->holdAbstractCallConnection(pCallData->m_callId,
                                                                         pCallData->m_sipDialog);
         }

         pCallData->m_pInst->pCallManager->holdLocalAbstractCallConnection(pCallData->m_callId);
         pCallData->m_bCallHoldInvoked = TRUE;
         sr = SIPX_RESULT_SUCCESS;
      }
      else
      {
         sr = SIPX_RESULT_INVALID_STATE;
      }
      
      sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
   }// no pCallData = call not found

   return sr;
}

SIPXTAPI_API SIPX_RESULT sipxCallUnhold(const SIPX_CALL hCall, int bTakeFocus)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallUnhold");
   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallUnhold hCall=%d, bTakeFocus=%d",
      hCall, bTakeFocus);


   SIPX_CALL_DATA* pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);
   if (pCallData)
   {
      if (pCallData->m_state != SIPX_INTERNAL_CALLSTATE_HELD &&
          pCallData->m_state != SIPX_INTERNAL_CALLSTATE_REMOTE_HELD &&
          pCallData->m_state != SIPX_INTERNAL_CALLSTATE_BRIDGED)
      {
         sr = SIPX_RESULT_INVALID_STATE;
      }
      else
      {
         // why do we set bInFocus here?, it should be done in reaction to event!
         pCallData->m_bInFocus = bTakeFocus | pCallData->m_bInFocus;
         // just posts message
         pCallData->m_pInst->pCallManager->unholdAbstractCallConnection(pCallData->m_callId,
                                                                        pCallData->m_sipDialog);  

         if (bTakeFocus)
         {
            // just posts message
            pCallData->m_pInst->pCallManager->unholdLocalAbstractCallConnection(pCallData->m_callId);
         }
         pCallData->m_bCallHoldInvoked = FALSE;
         sr = SIPX_RESULT_SUCCESS;
      }

      sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
   }// no pCallData = call not found

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallAnswer(const SIPX_CALL hCall, int bTakeFocus)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAnswer");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallAnswer hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA *pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pCallData)
   {
      if(!pCallData->m_remoteAddress.isNull() && !pCallData->m_sessionCallId.isNull())
      {
         // this call has a SipConnection, go on
         if (bTakeFocus)
         {
            pCallData->m_bInFocus = TRUE;
            // The hold event will remove the bInFocus param from 
            // the other connection
            pCallData->m_pInst->pCallManager->unholdLocalAbstractCallConnection(pCallData->m_callId);
         }
         
         pCallData->m_pInst->pCallManager->answerCallConnection(pCallData->m_callId);

         sr = SIPX_RESULT_SUCCESS;
      }// call found but no SipConnection, probably a call we created but didn't connect

      sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
   }// no pCallData = call not found

   return sr;
}

// returns Sip CallID
SIPXTAPI_API SIPX_RESULT sipxCallGetID(const SIPX_CALL hCall,
                                       char* szId,
                                       const size_t iMaxLength)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetID");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallGetID hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (szId)
   {
      UtlString sessionCallId;

      if (sipxCallGetCommonData(hCall, NULL, NULL, &sessionCallId, NULL, NULL))
      {
         if (iMaxLength > 0)
         {
            SAFE_STRNCPY(szId, sessionCallId, iMaxLength);
            sr = SIPX_RESULT_SUCCESS;
         }
      }// no pCallData = call not found
   }
      
   return sr;
}

// returns Line Uri of the call
SIPXTAPI_API SIPX_RESULT sipxCallGetLocalID(const SIPX_CALL hCall,
                                            char* szLineUri,
                                            const size_t iMaxLength)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetLocalID");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallGetLocalID hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (szLineUri)
   {
      UtlString fromUri;

      if (sipxCallGetCommonData(hCall, NULL, NULL, NULL, NULL, &fromUri))
      {
         if (iMaxLength > 0)
         {
            SAFE_STRNCPY(szLineUri, fromUri, iMaxLength);
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }
   
   return sr;
}

// returns remote address
SIPXTAPI_API SIPX_RESULT sipxCallGetRemoteID(const SIPX_CALL hCall,
                                             char* szRemoteAddress,
                                             const size_t iMaxLength)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetRemoteID");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallGetRemoteID hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (szRemoteAddress)
   {
      UtlString remoteAddress;

      if (sipxCallGetCommonData(hCall, NULL, NULL, NULL, &remoteAddress, NULL))
      {
         if (iMaxLength > 0)
         {
            SAFE_STRNCPY(szRemoteAddress, remoteAddress, iMaxLength);
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }
   
   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallGetContactID(const SIPX_CALL hCall,
                                              char* szContactAddress,
                                              const size_t iMaxLength)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetContactID");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallGetContactID hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);

   if (pData)
   {
      if (pData->m_pInst &&
         !pData->m_callId.isNull() && 
         !pData->m_remoteAddress.isNull())
      {
         SipDialog sipDialog(pData->m_sipDialog);
         sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

         Url contact;
         sipDialog.getLocalContact(contact);

         if (iMaxLength > 0)
         {
            SAFE_STRNCPY(szContactAddress, contact.toString().data(), iMaxLength);
            sr = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallGetConference(const SIPX_CALL hCall,
                                               SIPX_CONF* hConf) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetConference");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallGetConference hCall=%d", hCall);

   SIPX_RESULT sr = SIPX_RESULT_INVALID_ARGS;

   if (hConf)
   {
      if (hCall != SIPX_CALL_NULL)
      {
         *hConf = sipxCallGetConf(hCall);

         if (*hConf != SIPX_CONF_NULL)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
         else
         {
            sr = SIPX_RESULT_FAILURE;
         }
      }
   }
   
   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallConnect(SIPX_CALL hCall,
                                         const char* szAddress,
                                         SIPX_VIDEO_DISPLAY* const pDisplay,
                                         SIPX_SECURITY_ATTRIBUTES* const pSecurity,
                                         int bTakeFocus,
                                         SIPX_CALL_OPTIONS* options,
                                         const char* szSessionCallId)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallConnect");

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   char* pLocationHeader = NULL; // passed to CallManager

   // default values if options are not passed
   UtlBoolean bEnableLocationHeader = FALSE;
   int bandWidth = AUDIO_CODEC_BW_DEFAULT; // passed to CallManager
   SIPX_RTP_TRANSPORT rtpTransportOptions = SIPX_RTP_TRANSPORT_UDP; // passed to CallManager
   SIPX_CONTACT_ID contactId = 0; // passed to CallManager
   SIPX_TRANSPORT hTransport = SIPX_TRANSPORT_NULL;

   if (options)
   {
      if (options->cbSize == sizeof(SIPX_CALL_OPTIONS))
      {
         bEnableLocationHeader = options->sendLocation;
         bandWidth = options->bandwidthId;
         rtpTransportOptions = options->rtpTransportFlags;
         contactId = options->contactId;
         hTransport = options->hTransport;
      }
      else
      {
         // size mismatch
         assert(false);
         return SIPX_RESULT_INVALID_ARGS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallConnect hCall=%d szAddress=%s contactId=%d bEnableLocationHeader=%d bandWidth=%d",
      hCall, szAddress, contactId, bEnableLocationHeader, bandWidth);

   if (pDisplay)
   {
      assert(pDisplay->handle);
      if (!pDisplay->handle)
      {
         return SIPX_RESULT_INVALID_ARGS;
      }
   }
   assert(szAddress);
   if (!szAddress)
   {
      return SIPX_RESULT_INVALID_ARGS;
   }

   if (contactId < 0)
   {
      return SIPX_RESULT_INVALID_ARGS;
   }

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);
   if (pData)
   {
      SIPX_INSTANCE_DATA* pInst = pData->m_pInst;
      assert(pInst);
      assert(pData->m_remoteAddress.isNull());    // should be null

      if (pData->m_remoteAddress.isNull())
      {
         if (pInst)
         {
            // setup security
            if (pSecurity)
            {
               // augment the security attributes with the instance's security parameters
               SecurityHelper securityHelper;
               // generate a random key, if one isn't there
               if (strlen(pSecurity->getSrtpKey()) == 0)
               {
                  securityHelper.generateSrtpKey(*pSecurity);
               }
            }

            // maybe take focus
            if (bTakeFocus)
            {
               // just posts message
               pInst->pCallManager->holdLocalAbstractCallConnection(pData->m_callId);
               pData->m_bInFocus = TRUE;
            }

            // create sessionCallId
            pData->m_sessionCallId = szSessionCallId;
            if (pData->m_sessionCallId.isNull())
            {
               pData->m_sessionCallId = pInst->pCallManager->getNewSipCallId();
            }

            pData->m_hTransport = hTransport;

            // set location header
            pInst->lock.acquire();
            pLocationHeader = (bEnableLocationHeader) ? pInst->szLocationHeader : NULL;
            pInst->lock.release();

            OsStatus status = OS_FAILED;

            // connect just checks address and posts a message
            // cannot hold any locks here
            if (pData->m_hConf != SIPX_CONF_NULL)
            {
               // call is not part of conference
               status = pInst->pCallManager->connectCall(pData->m_callId, pData->m_sipDialog,
                  szAddress, pData->m_fullLineUrl.toString(), pData->m_sessionCallId,
                  pLocationHeader, contactId);
            }
            else
            {
               // call is part of conference
               status = pInst->pCallManager->connectConferenceCall(pData->m_callId, pData->m_sipDialog,
                  szAddress, pData->m_fullLineUrl.toString(), pData->m_sessionCallId,
                  pLocationHeader, contactId);
            }
            sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);

            if (status != OS_SUCCESS)
            {
               sr = SIPX_RESULT_BAD_ADDRESS;
            }
            else
            {
               sr = SIPX_RESULT_SUCCESS;
            }
         }
         else  // pInst == NULL
         {
            sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
            sr = SIPX_RESULT_FAILURE;
         }
      }
      else
      {
         sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
      }
   }// else call was not found

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallDestroy(SIPX_CALL* hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallDestroy");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallDestroy hCall=%d",
      hCall);

   SIPX_CONF hConf = sipxCallGetConf(*hCall);
   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   // if call is part of conference, remove it
   if (hConf != SIPX_CONF_NULL)
   {
      sr = sipxConferenceRemove(hConf, hCall);
   }
   else
   {
      // not part of conference, just drop it
      sr = sipxCallDrop(*hCall);
   }

   return sr;
}


// internal function used for testing only
SIPXTAPI_API SIPX_RESULT sipxCallGetConnectionId(const SIPX_CALL hCall,
                                                 int* connectionId)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetMediaConnectionId");
   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   *connectionId = -1;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);

   if (pData)        
   {
      // connectionId is cached, use it
      if (pData->m_connectionId != -1)
      {
         *connectionId = pData->m_connectionId;
         sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

         sr = SIPX_RESULT_SUCCESS;
      }
      else
      {
         // find media connectionId
         if (pData->m_pInst &&
             pData->m_pInst->pCallManager &&
             !pData->m_callId.isNull() && 
             !pData->m_remoteAddress.isNull())
         {
            XCpCallManager* pCallManager = pData->m_pInst->pCallManager;
            UtlString callId(pData->m_callId);
            SipDialog sipDialog(pData->m_sipDialog);

            sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

            pCallManager->getMediaConnectionId(callId, sipDialog, *connectionId);

            pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);
            if (pData)
            {
               pData->m_connectionId = *connectionId;
               sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
            }
            
            if (*connectionId != -1)
            {
               sr = SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
         }
      }
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallGetRequestURI(const SIPX_CALL hCall,
                                               char* szUri,
                                               const size_t iMaxLength)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetRequestURI");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallGetRequestURI hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE ;
   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);

   if (pData)
   {
      if (pData->m_pInst &&
          pData->m_pInst->pCallManager &&
          !pData->m_callId.isNull() &&
          !pData->m_remoteAddress.isNull()) 
      {
         SipDialog sipDialog(pData->m_sipDialog);
         sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

         Url requestUri;
         UtlString sUri;
         if (sipDialog.isLocalInitiatedDialog())
         {
            // outbound call
            sipDialog.getRemoteRequestUri(requestUri);
         }
         else
         {
            // inbound call
            sipDialog.getLocalRequestUri(requestUri);
         }
         requestUri.toString(sUri);

         if (iMaxLength)
         {
            SAFE_STRNCPY(szUri, sUri.data(), iMaxLength);
            sr = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallGetRemoteContact(const SIPX_CALL hCall,
                                                  char* szContact,
                                                  const size_t iMaxLength)
{
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallGetRemoteContact hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (szContact)
   {
      UtlString contactAddress;

      if (sipxCallGetCommonData(hCall, NULL, NULL, NULL, NULL, NULL, NULL, &contactAddress))
      {
         if (iMaxLength > 0)
         {
            SAFE_STRNCPY(szContact, contactAddress, iMaxLength);
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallGetRemoteUserAgent(const SIPX_CALL hCall,
                                                    char* szAgent,
                                                    const size_t iMaxLength)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallGetRemoteUserAgent");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallGetRemoteUserAgent hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);

   if (pData)
   {
      if (pData->m_pInst &&
          pData->m_pInst->pCallManager &&
          pData->m_sipDialog.isEstablishedDialog())
      {
         XCpCallManager* pCallManager = pData->m_pInst->pCallManager;
         UtlString callId(pData->m_callId);
         SipDialog sipDialog(pData->m_sipDialog);
         UtlString userAgent;

         sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

         pCallManager->getRemoteUserAgent(callId, sipDialog, userAgent);

         if (iMaxLength)
         {
            SAFE_STRNCPY(szAgent, userAgent.data(), iMaxLength);
            sr = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallStartTone(const SIPX_CALL hCall,
                                           const SIPX_TONE_ID toneId,
                                           const int bLocal,
                                           const int bRemote)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallStartTone");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallStartTone hCall=%d ToneId=%d bLocal=%d bRemote=%d",
      hCall, toneId, bLocal, bRemote);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pData)
   {
      // posts a message
      pData->m_pInst->pCallManager->audioToneStart(pData->m_callId, toneId, bLocal, bRemote);

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
      sr = SIPX_RESULT_SUCCESS;
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallStopTone(const SIPX_CALL hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallStopTone");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallStopTone hCall=%d",
      hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_CALL_DATA* pData = NULL;

   pData =  sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pData)
   {
      // posts a message
      pData->m_pInst->pCallManager->audioToneStop(pData->m_callId);
      sr = SIPX_RESULT_SUCCESS;

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallAudioPlayFileStart(const SIPX_CALL hCall,
                                                    const char* szFile,
                                                    const int bRepeat,
                                                    const int bLocal,
                                                    const int bRemote,
                                                    const int bMixWithMicrophone,
                                                    const float fVolumeScaling,
                                                    void* pCookie) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAudioPlayFileStart");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallAudioPlayFileStart hCall=%d File=%s bLocal=%d bRemote=%d bRepeat=%d bMixWithMic=%d, volScale=%f",
      hCall, szFile, bLocal, bRemote, bRepeat, bMixWithMicrophone, fVolumeScaling);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   // Massage volume scaling into range
   float fDownScaling = fVolumeScaling;
   if (fDownScaling > 1.0)
   {
      fDownScaling = 1.0;
   }
   else if (fDownScaling < 0.0)
   {
      fDownScaling = 0.0;
   }

   if (szFile)
   {
      SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

      if (pData)
      {
         // posts a message
         pData->m_pInst->pCallManager->audioFilePlay(pData->m_callId, szFile, bRepeat, bLocal, bRemote,
            bMixWithMicrophone, (int)(fDownScaling * 100.0), pCookie);

         sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);

         sr = SIPX_RESULT_SUCCESS;
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallAudioPlayFileStop(const SIPX_CALL hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAudioPlayFileStop");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallAudioPlayFileStop hCall=%d", hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pData)
   {
      // posts a message
      pData->m_pInst->pCallManager->audioStopPlayback(pData->m_callId);
      sr = SIPX_RESULT_SUCCESS;

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
   }

   return sr;
}

SIPXTAPI_API SIPX_RESULT sipxCallAudioPlaybackPause(const SIPX_CALL hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAudioPlaybackPause");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallAudioPlaybackPause hCall=%d", hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pData)
   {
      // posts a message
      pData->m_pInst->pCallManager->pauseAudioPlayback(pData->m_callId);
      sr = SIPX_RESULT_SUCCESS;

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
   }

   return sr;
}

SIPXTAPI_API SIPX_RESULT sipxCallAudioPlaybackResume(const SIPX_CALL hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAudioPlaybackResume");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallAudioPlaybackResume hCall=%d", hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pData)
   {
      // posts a message
      pData->m_pInst->pCallManager->resumeAudioPlayback(pData->m_callId);
      sr = SIPX_RESULT_SUCCESS;

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
   }

   return sr;
}



SIPXTAPI_API SIPX_RESULT sipxCallAudioRecordFileStart(const SIPX_CALL hCall,
                                                      const char* szFile) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAudioRecordFileStart");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallAudioRecordFileStart hCall=%d szFile=%s", hCall, szFile);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA *pInst;
   UtlString callId;
   UtlString remoteAddress;

   if (szFile && sipxCallGetCommonData(hCall, &pInst, &callId, NULL, &remoteAddress, NULL))
   {
      if (pInst->pCallManager->audioRecordStart(callId, szFile))
      {
         sr = SIPX_RESULT_SUCCESS;
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallAudioRecordFileStop(const SIPX_CALL hCall) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAudioRecordFileStop");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallAudioRecordFileStop hCall=%d", hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA *pInst;
   UtlString callId;
   UtlString remoteAddress;

   if (sipxCallGetCommonData(hCall, &pInst, &callId, NULL, &remoteAddress, NULL))
   {
      if (pInst->pCallManager->audioRecordStop(callId))
      {
         sr = SIPX_RESULT_SUCCESS;
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallPlayBufferStart(const SIPX_CALL hCall,
                                                 const void* pBuffer,
                                                 const int bufSize,
                                                 const int bufType,
                                                 const int bRepeat,
                                                 const int bLocal,
                                                 const int bRemote,
                                                 void* pCookie)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallPlayBufferStart");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallPlayBufferStart hCall=%d Buffer=%p Size=%d Type=%d bLocal=%d bRemote=%d bRepeat=%d",
      hCall, pBuffer, bufSize, bufType, bLocal, bRemote, bRepeat);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pData)
   {
      if (pBuffer)
      {
         SIPX_INSTANCE_DATA* pInst = pData->m_pInst;
         UtlString callId(pData->m_callId);
         sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);

         if (pInst)
         {
            pInst->pCallManager->audioBufferPlay(callId, pBuffer, bufSize, bufType, bRepeat,
               bLocal, bRemote, FALSE, 100, pCookie);
            sr = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
         sr = SIPX_RESULT_INVALID_ARGS;
      }
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallPlayBufferStop(const SIPX_CALL hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallPlayBufferStop");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallPlayBufferStop hCall=%d", hCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

   if (pData)
   {
      // just posts a message
      pData->m_pInst->pCallManager->audioStopPlayback(pData->m_callId);
      sr = SIPX_RESULT_SUCCESS;

      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallBlindTransfer(const SIPX_CALL hCall,
                                               const char* pszAddress)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallBlindTransfer");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallBlindTransfer hCall=%d Address=%s",
      hCall, pszAddress);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (pszAddress)
   {
      // first, get rid of any existing ghost connection
      SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

      if (pData)
      {
         SIPX_INSTANCE_DATA* pInst = pData->m_pInst;
         UtlString callId(pData->m_callId);
         SipDialog sipDialog(pData->m_sipDialog);
         sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);

         // call the transfer
         pInst->pCallManager->transferBlindAbstractCall(callId, sipDialog, pszAddress);
       
         sr = SIPX_RESULT_SUCCESS;
      }
      else
      {
         sr = SIPX_RESULT_INVALID_ARGS;
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallTransfer(const SIPX_CALL hSourceCall,
                                          const SIPX_CALL hTargetCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallTransfer");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallTransfer hSourceCall=%d hTargetCall=%d\n",
      hSourceCall, hTargetCall);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA *pInst1;
   SIPX_INSTANCE_DATA *pInst2;
   UtlString sourceCallId;
   SipDialog sourceSipDialog;
   UtlString targetCallId;
   SipDialog targetSipDialog;

   // get info about source call
   SIPX_CALL_DATA* pData = sipxCallLookup(hSourceCall, SIPX_LOCK_WRITE, stackLogger);
   if (pData)
   {
      pInst1 = pData->m_pInst;
      sourceCallId = pData->m_callId;
      sourceSipDialog = pData->m_sipDialog;
      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
   }
   else
   {
      return SIPX_RESULT_INVALID_ARGS;
   }

   // get info about target call
   pData = sipxCallLookup(hTargetCall, SIPX_LOCK_WRITE, stackLogger);
   if (pData)
   {
      pInst2 = pData->m_pInst;
      targetCallId = pData->m_callId;
      targetSipDialog = pData->m_sipDialog;
      sipxCallReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
   }
   else
   {
      return SIPX_RESULT_INVALID_ARGS;
   }

   // call transfer works only within the same instance
   if (pInst1 == pInst2)
   {
      // call the transfer            
      if (pInst1->pCallManager->transferConsultativeAbstractCall(sourceCallId, sourceSipDialog, targetCallId, targetSipDialog) == OS_SUCCESS)
      {
         sr = SIPX_RESULT_SUCCESS;
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}


SIPXTAPI_API SIPX_RESULT sipxCallGetAudioRtpSourceIds(const SIPX_CALL hCall,
                                                      unsigned int* iSendSSRC,
                                                      unsigned int* iReceiveSSRC) 
{
   SIPX_RESULT rc = SIPX_RESULT_NOT_IMPLEMENTED;

   return rc;
}


SIPXTAPI_API SIPX_RESULT sipxCallGetAudioRtcpStats(const SIPX_CALL hCall,
                                                   SIPX_RTCP_STATS* pStats)
{
   SIPX_RESULT rc = SIPX_RESULT_NOT_IMPLEMENTED;

   return rc;
}


SIPXTAPI_API SIPX_RESULT sipxCallLimitCodecPreferences(const SIPX_CALL hCall,
                                                       const SIPX_AUDIO_BANDWIDTH_ID audioBandwidth,
                                                       const char* szAudioCodecs,
                                                       const SIPX_VIDEO_BANDWIDTH_ID videoBandwidth,
                                                       const char* szVideoCodecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallLimitCodecPreferences");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallLimitCodecPreferences hCall=%d audioBandwidth=%d videoBandwidth=%d szAudioCodecs=\"%s\"",
      hCall,
      audioBandwidth,
      videoBandwidth,
      (szAudioCodecs != NULL) ? szAudioCodecs : "");

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   // Test bandwidth for legal values
   if (((audioBandwidth >= AUDIO_CODEC_BW_LOW && audioBandwidth <= AUDIO_CODEC_BW_HIGH) || audioBandwidth == AUDIO_CODEC_BW_DEFAULT) &&
      ((videoBandwidth >= VIDEO_CODEC_BW_LOW && videoBandwidth <= VIDEO_CODEC_BW_HIGH) || videoBandwidth == VIDEO_CODEC_BW_DEFAULT))
   {
      // get info about source call
      SIPX_CALL_DATA* pData = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger);
      if (pData)
      {
         SIPX_INSTANCE_DATA *pInst = pData->m_pInst;
         UtlString sCallId = pData->m_callId;
         SipDialog sipDialog = pData->m_sipDialog;
         sipxCallReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

         if (pInst->pCallManager->limitAbstractCallCodecPreferences(sCallId,
            (CP_AUDIO_BANDWIDTH_ID)audioBandwidth, szAudioCodecs,
            (CP_VIDEO_BANDWIDTH_ID)videoBandwidth, szVideoCodecs) == OS_SUCCESS)
         {
            if (pInst->pCallManager->renegotiateCodecsAbstractCallConnection(sCallId, sipDialog,
               (CP_AUDIO_BANDWIDTH_ID)audioBandwidth, szAudioCodecs,
               (CP_VIDEO_BANDWIDTH_ID)videoBandwidth, szVideoCodecs) == OS_SUCCESS)
            {
               sr = SIPX_RESULT_SUCCESS;
            }
         }
      }
      else
      {
         return SIPX_RESULT_INVALID_ARGS;
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}

SIPXTAPI_API SIPX_RESULT sipxCallMuteInput(const SIPX_CALL hCall, const int bMute)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallMuteInput");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallMuteInput hCall=%d bMute=%d", hCall, bMute);

   // Find Call
   SIPX_CALL_DATA* pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);
   if (pCallData)
   {
      SIPX_INSTANCE_DATA *pInst = pCallData->m_pInst;
      SipDialog sipDialog = pCallData->m_sipDialog;
      UtlString callId = pCallData->m_callId;
      sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);

      if (sipDialog.isEstablishedDialog())
      {
         if (bMute)
         {
            // local hold
            if (pInst->pCallManager->muteInputAbstractCallConnection(callId, sipDialog) == OS_SUCCESS)
            {
               return SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            // local unhold
            if (pInst->pCallManager->unmuteInputAbstractCallConnection(callId, sipDialog) == OS_SUCCESS)
            {
               return SIPX_RESULT_SUCCESS;
            }
         }
      }
   }

   return SIPX_RESULT_FAILURE;
}
