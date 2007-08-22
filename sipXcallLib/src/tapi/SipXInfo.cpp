//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
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

#include <net/SipUserAgent.h>
#include "tapi/SipXInfo.h"
#include "tapi/SipXHandleMap.h"
#include "tapi/SipXCall.h"
#include "os/OsDefs.h"
#include "tapi/SipXMessageObserver.h"
#include "cp/CallManager.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
SipXHandleMap gInfoHandleMap;  /**< Global Map of info handles */

// GLOBAL FUNCTIONS

// CHECKED
SIPX_INFO_DATA* sipxInfoLookup(const SIPX_INFO hInfo,
                               SIPX_LOCK_TYPE type,
                               const OsStackTraceLogger& oneBackInStack)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxInfoLookup", oneBackInStack);
   SIPX_INFO_DATA* pRC = NULL;
   OsStatus status = OS_FAILED;

   gInfoHandleMap.lock();
   pRC = (SIPX_INFO_DATA*)gInfoHandleMap.findHandle(hInfo);

   if (pRC)
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

   gInfoHandleMap.unlock();

   return pRC;
}

// CHECKED
void sipxInfoReleaseLock(SIPX_INFO_DATA* pData,
                         SIPX_LOCK_TYPE type,
                         const OsStackTraceLogger& oneBackInStack) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxInfoReleaseLock", oneBackInStack);
   OsStatus status;

   if (type != SIPX_LOCK_NONE)
   {
      switch (type)
      {
      case SIPX_LOCK_READ:
         status = pData->mutex.release();
         assert(status == OS_SUCCESS);
         break;
      case SIPX_LOCK_WRITE:
         status = pData->mutex.release();
         assert(status == OS_SUCCESS);
         break;
      default:
         break;
      }
   }
}

// CHECKED
void sipxInfoObjectFree(SIPX_INFO hInfo)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxInfoObjectFree");
   SIPX_INFO_DATA* pData = NULL;
   
   gInfoHandleMap.lock();
   pData = sipxInfoLookup(hInfo, SIPX_LOCK_WRITE, logItem);
   if (pData)
   {
      const void* pRC = gInfoHandleMap.removeHandle(hInfo);
      gInfoHandleMap.unlock();
      assert(pRC);

      sipxInfoFree(pData);
   }
   else
   {
      gInfoHandleMap.unlock(); // we can release lock now
   }
}

// CHECK
void sipxInfoFree(SIPX_INFO_DATA* pData)
{
   if (pData)
   {
      free((void*)pData->infoData.szFromURL);
      free((void*)pData->infoData.szUserAgent);
      free((void*)pData->infoData.szContentType);
      free((void*)pData->infoData.pContent);

      delete pData;
   }
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxCallSendInfo(SIPX_INFO* phInfo,
                                          const SIPX_CALL hCall,
                                          const char* szContentType,
                                          const char* szContent,
                                          const size_t nContentLength)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallSendInfo");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallSendInfo phInfo=%p hCall=%d contentType=%s content=%p contentLength=%d",
      phInfo, hCall, szContentType, szContent, (int) nContentLength);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (phInfo && szContent && nContentLength > 0)
   {
      SIPX_CALL_DATA* pCall = sipxCallLookup(hCall, SIPX_LOCK_READ, stackLogger); 

      if (pCall)
      {
         UtlString sessionCallId(pCall->sessionCallId);
         UtlString remoteAddress(pCall->remoteAddress);
         SIPX_INSTANCE_DATA* pInst = pCall->pInst;
         SIPX_LINE hLine = pCall->hLine;
         UtlString lineURI(pCall->lineURI);
         assert(pInst);
         sipxCallReleaseLock(pCall, SIPX_LOCK_READ, stackLogger);

         if (!sessionCallId.isNull())
         {
            SIPX_INFO_DATA* pInfoData = new SIPX_INFO_DATA();
            pInfoData->mutex.acquire();

            UtlBoolean res = gInfoHandleMap.allocHandle(*phInfo, pInfoData);

            if (res)
            {
               // handle allocation successful

               pInfoData->pInst = pInst;
               // Create Mutex
               pInfoData->infoData.nSize = sizeof(SIPX_INFO_INFO);
               pInfoData->infoData.hCall = hCall;
               pInfoData->infoData.hLine = hLine;
               pInfoData->infoData.szFromURL = SAFE_STRDUP(lineURI.data());
               pInfoData->infoData.nContentLength = nContentLength;
               pInfoData->infoData.szContentType = SAFE_STRDUP(szContentType);
               pInfoData->infoData.pContent = SAFE_STRDUP(szContent);

               SipSession* pSession = new SipSession(sessionCallId, remoteAddress, pInfoData->infoData.szFromURL);
               pInfoData->mutex.release();

               pInst->pSipUserAgent->addMessageObserver(*(pInst->pMessageObserver->getMessageQueue()), SIP_INFO_METHOD, 0, 1, 1, 0, 0, pSession, (void*)*phInfo);
               // message observer uses copy of pSession, it's safe to delete
               delete pSession;

               if (pInst->pCallManager->sendInfo(sessionCallId, remoteAddress, szContentType, nContentLength, szContent))
               {
                  sr = SIPX_RESULT_SUCCESS;
               }
               else
               {
                  sr = SIPX_RESULT_INVALID_STATE;
               }
            }
            else
            {
               // handle allocation failure
               OsSysLog::add(FAC_SIPXTAPI, PRI_ERR, 
                  "allocHandle failed to allocate a handle");
               delete pInfoData;
            }
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
   }

   return sr;
}
