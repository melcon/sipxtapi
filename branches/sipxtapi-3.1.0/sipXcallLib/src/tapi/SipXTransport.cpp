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

#include <os/OsTask.h>
#include <os/OsSysLog.h>
#include <utl/UtlVoidPtr.h>
#include <utl/UtlInt.h>
#include <utl/UtlHashMapIterator.h>
#include "tapi/SipXTransport.h"
#include "tapi/SipXHandleMap.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
SipXHandleMap gTransportHandleMap(1, SIPX_TRANSPORT_NULL);  /**< Global Map of External Transport object handles */

// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

// CHECKED
SIPX_TRANSPORT_DATA* sipxTransportLookup(const SIPX_TRANSPORT hTransport,
                                         SIPX_LOCK_TYPE type,
                                         const OsStackTraceLogger& oneBackInStack)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxTransportLookup", oneBackInStack);
   SIPX_TRANSPORT_DATA* pRC = NULL;
   OsStatus status = OS_FAILED;

   gTransportHandleMap.lock();
   pRC = (SIPX_TRANSPORT_DATA*)gTransportHandleMap.findHandle(hTransport);

   if (pRC)
   {
      switch (type)
      {
      case SIPX_LOCK_READ:
         status = pRC->pMutex->acquire();
         assert(status == OS_SUCCESS);
         break;
      case SIPX_LOCK_WRITE:
         status = pRC->pMutex->acquire();
         assert(status == OS_SUCCESS);
         break;
      default:
         break;
      }
   }

   gTransportHandleMap.unlock();

   return pRC;
}

// CHECKED
void sipxTransportReleaseLock(SIPX_TRANSPORT_DATA* pData,
                              SIPX_LOCK_TYPE type,
                              const OsStackTraceLogger& oneBackInStack) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxTransportReleaseLock", oneBackInStack);
   OsStatus status;

   assert(pData);
   if (pData && type != SIPX_LOCK_NONE)
   {
      switch (type)
      {
      case SIPX_LOCK_READ:
         status = pData->pMutex->release();
         assert(status == OS_SUCCESS);
         break;
      case SIPX_LOCK_WRITE:
         status = pData->pMutex->release();
         assert(status == OS_SUCCESS);
         break;
      default:
         break;
      }
   }
}

// CHECKED
void sipxTransportObjectFree(SIPX_TRANSPORT hTransport)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxTransportObjectFree");
   SIPX_TRANSPORT_DATA* pData = NULL;

   gTransportHandleMap.lock();
   pData = sipxTransportLookup(hTransport, SIPX_LOCK_WRITE, logItem);
   assert(pData);

   if (pData)
   {
      const void* pRC = gTransportHandleMap.removeHandle(hTransport); 
      assert(pRC);

      gTransportHandleMap.unlock();

      delete pData->pMutex;
      pData->pMutex = NULL;
      delete pData;
   }
   else
   {
      gTransportHandleMap.unlock();
   }
}

// CHECKED
void sipxTransportDestroyAll(const SIPX_INST hInst) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxTransportDestroyAll");
   gTransportHandleMap.lock();

   UtlHashMapIterator transportIterator(gTransportHandleMap);
   UtlInt*            pKey;
   UtlVoidPtr*        pValue;
   SIPX_TRANSPORT     hTransport;

   while ((pKey = (UtlInt*)transportIterator()))
   {
      pValue = (UtlVoidPtr*) gTransportHandleMap.findValue(pKey);
      hTransport = (SIPX_TRANSPORT)pValue->getValue();

      if (hTransport)
      {
         bool bRemove = false;

         SIPX_TRANSPORT_DATA* pData = sipxTransportLookup(hTransport, SIPX_LOCK_READ, logItem);
         if (pData)
         {
            if (pData->pInst == hInst)
            {
               bRemove = true;
            }
            sipxTransportReleaseLock(pData, SIPX_LOCK_READ, logItem);
         }

         if (bRemove)
         {
            sipxTransportObjectFree(hTransport);
         }
      }
   }

   gTransportHandleMap.unlock();
}

// CHECKED
const char* sipxTransportTypeToString(SIPX_TRANSPORT_TYPE type) 
{
   const char* szResult = "UNKNOWN";

   switch (type)
   {
   case TRANSPORT_UDP:
      szResult = MAKESTR(TRANSPORT_UDP);
      break;
   case TRANSPORT_TCP:
      szResult = MAKESTR(TRANSPORT_TCP);
      break;
   case TRANSPORT_TLS:
      szResult = MAKESTR(TRANSPORT_TLS);
      break;
   case TRANSPORT_CUSTOM:
      szResult = MAKESTR(TRANSPORT_CUSTOM);
      break;
   default:
      break;
   }

   return szResult;
}
