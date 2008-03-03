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
#include "tapi/SipXConference.h"
#include "tapi/SipXHandleMap.h"
#include "tapi/SipXCall.h"
#include "tapi/SipXCallEventListener.h"
#include "cp/CallManager.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
SipXHandleMap gConfHandleMap(1, SIPX_CONF_NULL);  /**< Global Map of conf handles */
extern SipXHandleMap gCallHandleMap;

// GLOBAL FUNCTIONS

/* ============================ FUNCTIONS ================================= */

// CHECKED
// WARNING: This relies on outside locking of conference SIPX_CONF_DATA
UtlBoolean sipxRemoveCallHandleFromConf(const SIPX_CONF hConf, 
                                        const SIPX_CALL hCall) 
{
   // global lock is not needed as lock on given conference is assumed
   UtlBoolean bFound = false;
   SIPX_CONF_DATA* pConfData = (SIPX_CONF_DATA*)gConfHandleMap.findHandle(hConf);

   if (validConfData(pConfData))
   {
      size_t idx;

      // First find the handle
      for (idx = 0; idx < pConfData->nCalls; idx++)
      {
         if (pConfData->hCalls[idx] == hCall)
         {
            bFound = true;
            break;
         }
      }

      if (bFound)
      {
         // decrease number of calls
         pConfData->nCalls--;

         // move calls
         for (; idx < pConfData->nCalls; idx++)
         {
            pConfData->hCalls[idx] = pConfData->hCalls[idx + 1];
         }

         // zero last call handle
         pConfData->hCalls[pConfData->nCalls] = SIPX_CALL_NULL;
      }
   }

   return bFound;
}

// CHECKED
SIPX_CONF_DATA* sipxConfLookup(const SIPX_CONF hConf,
                               SIPX_LOCK_TYPE type,
                               const OsStackTraceLogger& oneBackInStack) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfLookup", oneBackInStack);
   OsStatus status = OS_FAILED;
   SIPX_CONF_DATA* pRC = NULL;
   
   gConfHandleMap.lock();
   pRC = (SIPX_CONF_DATA*)gConfHandleMap.findHandle(hConf);

   if (validConfData(pRC))
   {
      switch (type)
      {
      case SIPX_LOCK_READ:
         status = pRC->mutex.acquire();
         assert(status == OS_SUCCESS); 
         break;
      case SIPX_LOCK_WRITE:
         status = pRC->mutex.acquire();
         assert(status == OS_SUCCESS); 
         break;
      default:
         break;
      }
   }
   else
   {
      pRC = NULL;
   }

   gConfHandleMap.unlock();

   return pRC;
}

// CHECKED
void sipxConfReleaseLock(SIPX_CONF_DATA* pData,
                         SIPX_LOCK_TYPE type,
                         const OsStackTraceLogger& oneBackInStack) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfReleaseLock", oneBackInStack);
   OsStatus status;

   if (type != SIPX_LOCK_NONE)
   {
      if (validConfData(pData))
      {
         switch (type)
         {
         case SIPX_LOCK_READ:
            status = pData->mutex.release();
            assert(status == OS_SUCCESS);
            break ;
         case SIPX_LOCK_WRITE:
            status = pData->mutex.release();
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

// CHECKED
void sipxConfFree(const SIPX_CONF hConf) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfFree");
   SIPX_CONF_DATA* pData = NULL;
   
   gConfHandleMap.lock();
   pData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, logItem);
   assert(pData);

   if (pData)
   {
      assert(pData->nCalls == 0);
      const void* pRC = gConfHandleMap.removeHandle(hConf);
      assert(pRC);
      gConfHandleMap.unlock();

      // decrease conference counter
      pData->pInst->lock.acquire();
      pData->pInst->nConferences--;
      assert(pData->pInst->nConferences >= 0);
      pData->pInst->lock.release();

      // drop conference shell call
      pData->pInst->pCallManager->drop(pData->confCallId);
      delete pData;
   }
   else
   {
      // pData is NULL
      gConfHandleMap.unlock();
   }
}

// CHECKED
void sipxConferenceDestroyAll(const SIPX_INST hInst) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceDestroyAll");
   gConfHandleMap.lock();

   UtlHashMapIterator pubIter(gConfHandleMap);
   UtlInt* pKey;
   UtlVoidPtr* pValue;
   SIPX_CONF hConf;

   while ((pKey = (UtlInt*)pubIter()))
   {
      pValue = (UtlVoidPtr*)gConfHandleMap.findValue(pKey);
      assert(pValue);

      hConf = (SIPX_CONF)pValue->getValue();

      if (hConf)
      {
         bool bRemove = false;

         SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, logItem);
         if (pData)
         {
            if (pData->pInst == hInst)
            {
               bRemove = true;
            }
            sipxConfReleaseLock(pData, SIPX_LOCK_READ, logItem);
         }

         if (bRemove)
         {
            sipxConferenceDestroy(hConf);
         }
      }
   }

   gConfHandleMap.unlock();
}

// CHECKED
UtlBoolean validConfData(const SIPX_CONF_DATA* pData) 
{
   UtlBoolean bValid = FALSE;

   if (pData)
   {
      bValid = TRUE;
   }

   return bValid;
}

// CHECKED
UtlBoolean sipxAddCallHandleToConf(const SIPX_CALL hCall,
                                   const SIPX_CONF hConf)
{
   UtlBoolean res = FALSE;    
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxAddCallHandleToConf");

   SIPX_CONF_DATA* pConfData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, logItem);
   if (pConfData)
   {
      pConfData->hCalls[pConfData->nCalls++] = hCall;
      sipxConfReleaseLock(pConfData, SIPX_LOCK_WRITE, logItem);

      SIPX_CALL_DATA* pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, logItem);

      if (pCallData)
      {
         pCallData->hConf = hConf;
         res = TRUE;

         sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, logItem);
      }
      else
      {
         // restore conference nCalls if pCallData not found
         pConfData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, logItem);

         if (pConfData)
         {
            pConfData->hCalls[--pConfData->nCalls] = SIPX_CALL_NULL;
            sipxConfReleaseLock(pConfData, SIPX_LOCK_WRITE, logItem);
         }
      }
   }

   return res;
}

/***************************************************************************
* Public Conference Related Functions
***************************************************************************/

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceGetCalls(const SIPX_CONF hConf,
                                                SIPX_CALL hCalls[],
                                                const size_t iMax,
                                                size_t* nActual)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceGetCalls");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceGetCalls hConf=%d",
      hConf);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   *nActual = 0;

   if (hConf > SIPX_CONF_NULL && iMax > 0)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);
      if (pData)
      {
         size_t idx = 0;
         for (; (idx < pData->nCalls) && (idx < iMax); idx++)
         {
            hCalls[idx] = pData->hCalls[idx];
         }
         *nActual = idx;

         sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

         rc = SIPX_RESULT_SUCCESS;
      }
      else
      {
         rc = SIPX_RESULT_FAILURE;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceCreate(const SIPX_INST hInst,
                                              SIPX_CONF *phConference)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceCreate");
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceCreate hInst=%p phConference=%p",
      hInst, phConference);

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      if (phConference)
      {
         *phConference = SIPX_CONF_NULL;

         SIPX_CONF_DATA* pData = new SIPX_CONF_DATA();
         pData->mutex.acquire();
         UtlBoolean res = gConfHandleMap.allocHandle(*phConference, pData);

         if (res)
         {
            // Increment conference counter
            pInst->lock.acquire();
            pInst->nConferences++;
            pInst->lock.release();

            // Init conference data
            pData->pInst = pInst;

            // create shell call for conference
            pData->pInst->pCallManager->getNewCallId(&pData->confCallId);
            pData->pInst->pCallManager->createCall(&pData->confCallId);

            pData->mutex.release();
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            // handle allocation failure
            OsSysLog::add(FAC_SIPXTAPI, PRI_ERR, 
               "allocHandle failed to allocate a handle");
            delete pData;
         }
      }
      else
      {
         rc = SIPX_RESULT_INVALID_ARGS;
      }
   }
   else
   {
      rc = SIPX_RESULT_INVALID_ARGS;
   }
   
   return rc;
}

// CHECKED, verify split failure
SIPXTAPI_API SIPX_RESULT sipxConferenceJoin(const SIPX_CONF hConf,
                                            const SIPX_CALL hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceJoin");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceJoin hConf=%d hCall=%d",
      hConf, hCall);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   UtlBoolean bDoSplit = FALSE;
   UtlString sourceSessionCallId;
   UtlString sourceCallId;
   UtlString sourceAddress;
   UtlString targetCallId;
   SIPX_INSTANCE_DATA* pInst = NULL;

   if (hConf > SIPX_CONF_NULL && hCall > SIPX_CALL_NULL)
   {
      SIPX_CONF_DATA* pConfData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, stackLogger);

      if (pConfData)
      {
         UtlString confCallId(pConfData->confCallId);
         sipxConfReleaseLock(pConfData, SIPX_LOCK_WRITE, stackLogger);

         // conference was found as is locked
         SIPX_CALL_DATA * pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

         if (pCallData)
         {
            // call was found and is locked
            if (pCallData->hConf == SIPX_CALL_NULL)
            {
               // call is not yet in conference

               // we need to split connection from old CpPeerCall
               // and join it into conference CpPeerCall
               if ((pCallData->state == SIPX_INTERNAL_CALLSTATE_REMOTE_HELD) ||
                  (pCallData->state == SIPX_INTERNAL_CALLSTATE_HELD))
               {
                  // Mark data for split/drop below
                  bDoSplit = TRUE;
                  sourceSessionCallId = pCallData->sessionCallId;
                  sourceCallId = pCallData->callId;
                  sourceAddress = pCallData->remoteAddress;
                  targetCallId = confCallId; // conference shell CallId
                  pInst = pCallData->pInst;

                  // Update data structures
                  pCallData->callId = targetCallId; // call will be moved to conference
                  pCallData->hConf = hConf; // store conference handle
               }
               else
               {
                  rc = SIPX_RESULT_INVALID_STATE;
               }
            }
            else
            {
               // call is already in a conference
               rc = SIPX_RESULT_INVALID_STATE;
            }

            sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
         }
      }

      if (bDoSplit)
      {
         // Do the split
         PtStatus status = pInst->pCallManager->split(sourceSessionCallId, sourceAddress, targetCallId);
         if (status != PT_SUCCESS)
         {
            rc = SIPX_RESULT_FAILURE;
         }
         else
         {
            rc = SIPX_RESULT_SUCCESS;

            // Add call to conference handle
            sipxAddCallHandleToConf(hCall, hConf);
         }
         // If the call fails -- hard to recover, drop the call anyways.
         // If split fails, call will be in inconsistent state as we already changed
         // pCallData
         pInst->pCallManager->drop(sourceCallId);
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceSplit(const SIPX_CONF hConf,
                                             const SIPX_CALL hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceSplit");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceSplit hConf=%d hCall=%d",
      hConf, hCall);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   UtlBoolean doSplit = FALSE;
   UtlString sourceSessionCallId;
   UtlString sourceAddress;
   UtlString targetCallId;
   SIPX_INSTANCE_DATA *pInst = NULL;

   if (hConf > SIPX_CONF_NULL && hCall > SIPX_CALL_NULL)
   {
      SIPX_CONF_DATA* pConfData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, stackLogger);

      if (pConfData)
      {
         // conference was found
         sipxConfReleaseLock(pConfData, SIPX_LOCK_WRITE, stackLogger);

         SIPX_CALL_DATA* pCallData = sipxCallLookup(hCall, SIPX_LOCK_WRITE, stackLogger);

         if (pCallData)
         {
            // call was found
            if ((pCallData->state == SIPX_INTERNAL_CALLSTATE_REMOTE_HELD) || 
                (pCallData->state == SIPX_INTERNAL_CALLSTATE_HELD))
            {
               doSplit = TRUE;
               // Record data for split
               pInst = pCallData->pInst;
               sourceSessionCallId = pCallData->sessionCallId;
               sourceAddress = pCallData->remoteAddress;

               // Create a CpPeerCall call to hold connection
               // creates callid and posts message
               pCallData->pInst->pCallManager->createCall(&targetCallId);

               pCallData->callId = targetCallId; // set new callId
               pCallData->hConf = SIPX_CALL_NULL;
            }
            else
            {
               rc = SIPX_RESULT_INVALID_STATE;
            }

            sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
         }
      }

      // Initiate Split
      if (doSplit)
      {
         PtStatus status = pInst->pCallManager->split(sourceSessionCallId, sourceAddress, targetCallId);
         if (status != PT_SUCCESS)
         {
            // split failure
            rc = SIPX_RESULT_FAILURE;
         }
         else
         {
            // Remove from conference handle
            sipxRemoveCallHandleFromConf(hConf, hCall);

            // split successful
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceAdd(const SIPX_CONF hConf,
                                           const SIPX_LINE hLine,
                                           const char* szAddress,
                                           SIPX_CALL* phNewCall,
                                           SIPX_CONTACT_ID contactId,
                                           SIPX_VIDEO_DISPLAY* const pDisplay,
                                           SIPX_SECURITY_ATTRIBUTES* const pSecurity,
                                           int bTakeFocus,
                                           SIPX_CALL_OPTIONS* options)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceAdd");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceAdd hConf=%d hLine=%d szAddress=%s contactId=%d, pDisplay=%p ",
      hConf, hLine, szAddress, contactId, pDisplay);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, stackLogger);
   if (pData)
   {
      SIPX_INSTANCE_DATA* pInst = pData->pInst;
      size_t nCalls = pData->nCalls;
      UtlString confCallId(pData->confCallId);
      sipxConfReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);

      if (pInst)
      {
         // conference was found
         if (nCalls < CONF_MAX_CONNECTIONS && 
             pInst->pCallManager->canAddConnection(confCallId))
         {
            UtlString sessionCallId;
            // create session call id for conference call
            pInst->pCallManager->getNewSessionId(&sessionCallId);

            // call can be added, create it with confCallId
            SIPX_RESULT res = sipxCallCreateHelper(pInst,
               hLine,
               NULL,
               hConf,
               phNewCall,
               confCallId,
               sessionCallId,
               false,
               false);

            if (res == SIPX_RESULT_SUCCESS)
            {
               // call was created, add it to conference
               sipxAddCallHandleToConf(*phNewCall, hConf);

               // fire dialtone event manually - used for conferences
               pInst->pCallEventListener->OnDialTone(CpCallStateEvent(sessionCallId,
                  confCallId,
                  SipSession(),
                  NULL,
                  CALLSTATE_CAUSE_CONFERENCE));

               // connect call
               rc = sipxCallConnect(*phNewCall, szAddress, pDisplay, pSecurity,
                  bTakeFocus, options, sessionCallId);
            }
         }
         else
         {
            rc = SIPX_RESULT_OUT_OF_RESOURCES;
         }
      }      
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceRemove(const SIPX_CONF hConf,
                                              SIPX_CALL* hCall)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceRemove");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceRemove hConf=%d hCall=%d",
      hConf, hCall);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;

   if (hConf > SIPX_CONF_NULL && hCall > SIPX_CALL_NULL)
   {
      SIPX_CONF_DATA* pConfData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, stackLogger);

      if (pConfData)
      {
         SIPX_INSTANCE_DATA* pInst = pConfData->pInst;
         sipxConfReleaseLock(pConfData, SIPX_LOCK_WRITE, stackLogger);

         if (pInst)
         {
            SIPX_CALL_DATA* pCallData = sipxCallLookup(*hCall, SIPX_LOCK_WRITE, stackLogger);

            if (pCallData)
            {
               UtlString sessionCallId(pCallData->sessionCallId);
               UtlString remoteAddress(pCallData->remoteAddress);
               SIPX_CONF hCallConf = pCallData->hConf;
               assert(hCallConf != SIPX_CONF_NULL);

               sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);

               if (hCallConf == hConf)
               {
                  // remove call handle from conference
                  sipxRemoveCallHandleFromConf(hConf, *hCall);
                  pInst->pCallManager->dropConnection(sessionCallId, remoteAddress);

                  *hCall = SIPX_CALL_NULL;
                  rc = SIPX_RESULT_SUCCESS;
               }
            }
         }         
      }
      else
      {
         // Either the call or conf doesn't exist
         rc = SIPX_RESULT_FAILURE;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceHold(const SIPX_CONF hConf,
                                            int bBridging)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceHold");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceHold hConf=%d bBridging=%d",
      hConf,
      bBridging);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (hConf > SIPX_CONF_NULL)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, stackLogger);

      if (pData)
      {
         sr = SIPX_RESULT_INVALID_STATE;

         if (pData->hCalls > 0)
         {
            if (bBridging)
            {
               // use bridged (local) hold, just reposts message to thread
               pData->pInst->pCallManager->holdLocalTerminalConnection(pData->confCallId);
               pData->confHoldState = CONF_STATE_BRIDGING_HOLD;
               sr = SIPX_RESULT_SUCCESS;
            }
            else
            {
               // just reposts message to thread
               pData->pInst->pCallManager->holdAllTerminalConnections(pData->confCallId);
               pData->confHoldState = CONF_STATE_NON_BRIDGING_HOLD;
               sr = SIPX_RESULT_SUCCESS;
            }
         }

         sipxConfReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceUnhold(const SIPX_CONF hConf)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceUnhold");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceUnHold hConf=%d",
      hConf);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (hConf > SIPX_CONF_NULL)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         sr = SIPX_RESULT_INVALID_STATE;

         if (pData->hCalls > 0)
         {
            if (pData->confHoldState == CONF_STATE_BRIDGING_HOLD)
            {
               // just posts a message
               pData->pInst->pCallManager->unholdLocalTerminalConnection(pData->confCallId);
               pData->confHoldState = CONF_STATE_UNHELD;
               sr = SIPX_RESULT_SUCCESS;
            }
            else if (pData->confHoldState == CONF_STATE_NON_BRIDGING_HOLD)
            {
               // just posts a message
               pData->pInst->pCallManager->unholdAllTerminalConnections(pData->confCallId);
               pData->confHoldState = CONF_STATE_UNHELD;
               sr = SIPX_RESULT_SUCCESS;
            }
         }         

         sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferencePlayAudioFileStart(const SIPX_CONF hConf,
                                                          const char* szFile,
                                                          const int bRepeat,
                                                          const int bLocal,
                                                          const int bRemote,
                                                          const int bMixWithMic,
                                                          const float fVolumeScaling)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferencePlayAudioFileStart");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferencePlayAudioFileStart hConf=%d File=%s bLocal=%d bRemote=%d bRepeat=%d",
      hConf, szFile, bLocal, bRemote, bRepeat);

   SIPX_RESULT sr = SIPX_RESULT_INVALID_ARGS;

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

   if (hConf > SIPX_CONF_NULL && szFile && (bLocal || bRemote))
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, stackLogger);

      if (pData)
      {
         if (pData->nCalls > 0)
         {
            // conference was found, just repost message
            pData->nNumFilesPlaying++;
            pData->pInst->pCallManager->audioPlay(pData->confCallId, szFile, bRepeat, bLocal, bRemote, bMixWithMic, (int) (fDownScaling * 100.0));

            sr = SIPX_RESULT_SUCCESS;
         }         
         sipxConfReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferencePlayAudioFileStop(const SIPX_CONF hConf)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferencePlayAudioFileStop");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferencePlayAudioFileStop hConf=%d", hConf);

   SIPX_RESULT sr = SIPX_RESULT_INVALID_ARGS;

   if (hConf)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_WRITE, stackLogger);

      if (pData)
      {
         // conference was found, just repost message
         if (pData->nCalls > 0)
         {
            pData->nNumFilesPlaying--;
            pData->pInst->pCallManager->audioStop(pData->confCallId);
            sr = SIPX_RESULT_SUCCESS;
         }

         sipxConfReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceDestroy(SIPX_CONF hConf)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceDestroy");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceDestroy hConf=%d",
      hConf);

   SIPX_CALL hCalls[CONF_MAX_CONNECTIONS];
   size_t nCalls;
   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;

   if (hConf > SIPX_CONF_NULL)
   {
      int nNumFilesPlaying = 0;
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         nNumFilesPlaying = pData->nNumFilesPlaying;
         sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }

      if (nNumFilesPlaying <= 0)
      {
         // Get a snapshot of the calls, drop the connections, remove the conf handle,
         // and THEN whack the call -- otherwise whacking the calls will force updates
         // into SIPX_CONF_DATA structure (work that isn't needed).
         sipxConferenceGetCalls(hConf, hCalls, CONF_MAX_CONNECTIONS, &nCalls);

         for (size_t idx = 0; idx < nCalls; idx++)
         {
            sipxConferenceRemove(hConf, &hCalls[idx]);
         }

         sipxConfFree(hConf);

         rc = SIPX_RESULT_SUCCESS;
      }
      else
      {
         rc = SIPX_RESULT_BUSY;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceGetEnergyLevels(const SIPX_CONF hConf,
                                                       int* iInputEnergyLevel,
                                                       int* iOutputEnergyLevel) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceGetEnergyLevels");
   SIPX_RESULT sr = SIPX_RESULT_INVALID_ARGS;

   if (hConf > SIPX_CONF_NULL)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         if (pData->pInst && pData->pInst->pCallManager && !pData->confCallId.isNull())
         {
            CallManager* pCallManager = pData->pInst->pCallManager;
            UtlString callId = pData->confCallId;

            sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

            if (pCallManager->getAudioEnergyLevels(callId, *iInputEnergyLevel, *iOutputEnergyLevel))
            {
               sr = SIPX_RESULT_SUCCESS;
            }
            else
            {
               sr = SIPX_RESULT_FAILURE;
            }
         }
         else
         {
            sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
            sr = SIPX_RESULT_FAILURE;
         }
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConferenceLimitCodecPreferences(const SIPX_CONF hConf,
                                                             const SIPX_AUDIO_BANDWIDTH_ID audioBandwidth,
                                                             const SIPX_VIDEO_BANDWIDTH_ID videoBandwidth,
                                                             const char* szVideoCodecName) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceLimitCodecPreferences");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceLimitCodecPreferences hCall=%d audioBandwidth=%d videoBandwidth=%d szVideoCodecName=\"%s\"",
      hConf,
      audioBandwidth,
      videoBandwidth,
      (szVideoCodecName) ? szVideoCodecName : "");

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   // Test bandwidth for legal values
   if (((audioBandwidth >= AUDIO_CODEC_BW_LOW && audioBandwidth <= AUDIO_CODEC_BW_HIGH) || audioBandwidth == AUDIO_CODEC_BW_DEFAULT) &&
      ((videoBandwidth >= VIDEO_CODEC_BW_LOW && videoBandwidth <= VIDEO_CODEC_BW_HIGH) || videoBandwidth == VIDEO_CODEC_BW_DEFAULT) &&
      (hConf != SIPX_CONF_NULL))
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         if (!pData->confCallId.isNull())
         {
            // just reposts message
            pData->pInst->pCallManager->limitCodecPreferences(pData->confCallId, audioBandwidth, videoBandwidth, szVideoCodecName);
            pData->pInst->pCallManager->silentRemoteHold(pData->confCallId);
            pData->pInst->pCallManager->renegotiateCodecsAllTerminalConnections(pData->confCallId);

            sr = SIPX_RESULT_SUCCESS;
         }
         else
         {
            sr = SIPX_RESULT_INVALID_ARGS;
         }

         sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }      
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}

