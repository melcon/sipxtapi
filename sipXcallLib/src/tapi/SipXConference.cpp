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
#include <utl/UtlInt.h>
#include <utl/UtlVoidPtr.h>
#include <utl/UtlHashMapIterator.h>
#include "tapi/SipXConference.h"
#include "tapi/SipXHandleMap.h"
#include "tapi/SipXCall.h"
#include "tapi/SipXCallEventListener.h"
#include "cp/XCpCallManager.h"

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

SIPX_CONF sipxConfLookupHandleByConfId(const UtlString& confID, SIPX_INST pInst)
{
   SIPX_CONF hConf = SIPX_CONF_NULL;
   SIPX_CONF_DATA* pData = NULL;
   OsStatus status;

   gConfHandleMap.lock();
   // control iterator scope
   {
      UtlHashMapIterator iter(gConfHandleMap);

      UtlInt* pIndex = NULL;
      UtlVoidPtr* pObj = NULL;

      while ((pIndex = dynamic_cast<UtlInt*>(iter())))       
      {
         pObj = dynamic_cast<UtlVoidPtr*>(gConfHandleMap.findValue(pIndex));

         assert(pObj); // if it's NULL, then it's a bug
         if (pObj)
         {
            pData = (SIPX_CONF_DATA*)pObj->getValue();
            assert(pData);

            if (pData)
            {
               status = pData->mutex.acquire();
               assert(status == OS_SUCCESS);

               if (pData->confCallId.compareTo(confID) == 0 && 
                  pData->pInst == pInst)
               {
                  hConf = pIndex->getValue();
                  status = pData->mutex.release();
                  assert(status == OS_SUCCESS);
                  break;
               }

               status = pData->mutex.release();
               assert(status == OS_SUCCESS);
            }            
         }
      }
   }
   gConfHandleMap.unlock();

   return hConf;
}

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
      pData->pInst->pCallManager->dropConference(pData->confCallId);
      delete pData;
   }
   else
   {
      // pData is NULL
      gConfHandleMap.unlock();
   }
}


void sipxConferenceDestroyAll(const SIPX_INST hInst) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceDestroyAll");
   gConfHandleMap.lock();

   UtlHashMapIterator pubIter(gConfHandleMap);
   UtlInt* pKey;
   UtlVoidPtr* pValue;
   SIPX_CONF hConf;

   while ((pKey = dynamic_cast<UtlInt*>(pubIter())))
   {
      pValue = dynamic_cast<UtlVoidPtr*>(gConfHandleMap.findValue(pKey));
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


UtlBoolean validConfData(const SIPX_CONF_DATA* pData) 
{
   UtlBoolean bValid = FALSE;

   if (pData)
   {
      bValid = TRUE;
   }

   return bValid;
}


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
         pCallData->m_hConf = hConf;
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


SIPXTAPI_API SIPX_RESULT sipxConferenceCreate(const SIPX_INST hInst,
                                              SIPX_CONF *phConference)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceCreate");
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceCreate hInst=%p phConference=%p",
      hInst, phConference);

   SIPX_INSTANCE_DATA* pInst = SAFE_PTR_CAST(SIPX_INSTANCE_DATA, hInst);

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
            pData->pInst->pCallManager->createConference(pData->confCallId);

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
            sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
         }
      }
   }

   return rc;
}


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
            sipxCallReleaseLock(pCallData, SIPX_LOCK_WRITE, stackLogger);
         }
      }
   }

   return rc;
}


SIPXTAPI_API SIPX_RESULT sipxConferenceAdd(const SIPX_CONF hConf,
                                           const SIPX_LINE hLine,
                                           const char* szAddress,
                                           SIPX_CALL* phNewCall,
                                           SIPX_CONTACT_ID contactId,
                                           SIPX_VIDEO_DISPLAY* const pDisplay,
                                           SIPX_SECURITY_ATTRIBUTES* const pSecurity,
                                           SIPX_FOCUS_CONFIG takeFocus,
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

      if (pInst && nCalls < CONF_MAX_CONNECTIONS)
      {
         // conference was found
         // create session call id for conference call
         UtlString sessionCallId = pInst->pCallManager->getNewSipCallId();         

         // call can be added, create it with confCallId
         SIPX_RESULT res = sipxCallCreateHelper(pInst,
            hLine,
            NULL,
            hConf,
            phNewCall,
            confCallId,
            sessionCallId,
            true);// bIsConferenceCall

         if (res == SIPX_RESULT_SUCCESS)
         {
            // call was created, add it to conference
            sipxAddCallHandleToConf(*phNewCall, hConf);

            // connect call
            rc = sipxCallConnect(*phNewCall, szAddress, pDisplay, pSecurity,
               takeFocus, options, sessionCallId);
         }
         else
         {
            rc = SIPX_RESULT_OUT_OF_RESOURCES;
         }
      }      
   }

   return rc;
}


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
         UtlString conferenceId(pConfData->confCallId);
         sipxConfReleaseLock(pConfData, SIPX_LOCK_WRITE, stackLogger);
      }
      else
      {
         // Either the call or conf doesn't exist
         rc = SIPX_RESULT_FAILURE;
      }
   }

   return rc;
}

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
               pData->pInst->pCallManager->holdLocalAbstractCallConnection(pData->confCallId);
               pData->confHoldState = CONF_STATE_BRIDGING_HOLD;
               sr = SIPX_RESULT_SUCCESS;
            }
            else
            {
               pData->pInst->pCallManager->holdAllConferenceConnections(pData->confCallId);
               pData->confHoldState = CONF_STATE_NON_BRIDGING_HOLD;
               sr = SIPX_RESULT_SUCCESS;
            }
         }

         sipxConfReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
      }
   }

   return sr;
}


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
               pData->pInst->pCallManager->unholdLocalAbstractCallConnection(pData->confCallId);
               pData->confHoldState = CONF_STATE_UNHELD;
               sr = SIPX_RESULT_SUCCESS;
            }
            else if (pData->confHoldState == CONF_STATE_NON_BRIDGING_HOLD)
            {
               // just posts a message
               pData->pInst->pCallManager->unholdAllConferenceConnections(pData->confCallId);
               pData->confHoldState = CONF_STATE_UNHELD;
               sr = SIPX_RESULT_SUCCESS;
            }
         }   

         sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }
   }

   return sr;
}


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
            pData->pInst->pCallManager->audioFilePlay(pData->confCallId, szFile, bRepeat,
               bLocal, bRemote, bMixWithMic, (int) (fDownScaling * 100.0));

            sr = SIPX_RESULT_SUCCESS;
         }         
         sipxConfReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
      }
   }

   return sr;
}


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
            pData->pInst->pCallManager->audioStopPlayback(pData->confCallId);
            sr = SIPX_RESULT_SUCCESS;
         }

         sipxConfReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);
      }
   }

   return sr;
}

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

SIPXTAPI_API SIPX_RESULT sipxConferenceLimitCodecPreferences(const SIPX_CONF hConf,
                                                             const char* szAudioCodecNames,
                                                             const char* szVideoCodecNames) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceLimitCodecPreferences");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceLimitCodecPreferences hCall=%d szAudioCodecNames=\"%s\"",
      hConf,
      (szAudioCodecNames) ? szAudioCodecNames : "");

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   // Test bandwidth for legal values
   if (hConf != SIPX_CONF_NULL)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         if (!pData->confCallId.isNull())
         {
            // just reposts message
            pData->pInst->pCallManager->limitAbstractCallCodecPreferences(pData->confCallId,
               szAudioCodecNames,
               szVideoCodecNames);
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

SIPXTAPI_API SIPX_RESULT sipxConferenceRenegotiateCodecPreferences(const SIPX_CONF hConf,
                                                                   const char* szAudioCodecNames,
                                                                   const char* szVideoCodecNames) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceRenegotiateCodecPreferences");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceRenegotiateCodecPreferences hCall=%d szAudioCodecNames=\"%s\"",
      hConf,
      (szAudioCodecNames) ? szAudioCodecNames : "");

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   // Test bandwidth for legal values
   if (hConf != SIPX_CONF_NULL)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         if (!pData->confCallId.isNull())
         {
            // just reposts message
            pData->pInst->pCallManager->renegotiateCodecsAllConferenceConnections(pData->confCallId,
               szAudioCodecNames,
               szVideoCodecNames);

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

SIPXTAPI_API SIPX_RESULT sipxConferenceAudioRecordFileStart(const SIPX_CONF hConf, const char* szFile)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallAudioRecordFileStart");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceAudioRecordFileStart hConf=%d szFile=%s", hConf, szFile);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   
   if (hConf > SIPX_CONF_NULL && szFile)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         // conference was found, just repost message
         pData->pInst->pCallManager->audioRecordStart(pData->confCallId, szFile);
         sr = SIPX_RESULT_SUCCESS;
         sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }
   }

   return sr;
}

SIPXTAPI_API SIPX_RESULT sipxConferenceAudioRecordFileStop(const SIPX_CONF hConf)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConferenceAudioRecordFileStop");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConferenceAudioRecordFileStop hConf=%d", hConf);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (hConf > SIPX_CONF_NULL)
   {
      SIPX_CONF_DATA* pData = sipxConfLookup(hConf, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         // conference was found, just repost message
         pData->pInst->pCallManager->audioRecordStop(pData->confCallId);
         sr = SIPX_RESULT_SUCCESS;
         sipxConfReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }
   }

   return sr;
}
