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

#ifndef SipXTransport_h__
#define SipXTransport_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include "tapi/SipXCore.h"

// DEFINES
#define MAX_TRANSPORT_NAME 32

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
class OsStackTraceLogger;

// STRUCTS
// TYPEDEFS

class SIPX_TRANSPORT_DATA
{
public:
   SIPX_TRANSPORT_DATA() 
   {
      pInst = NULL;
      bIsReliable = FALSE;
      iLocalPort = -1;
      pFnWriteProc = NULL;
      pMutex = NULL;
      hTransport = 0;
      pUserData = NULL;
      bRouteByUser = TRUE;
      memset(szLocalIp, 0, sizeof(szLocalIp));
      memset(szTransport, 0, sizeof(szTransport));
      memset(cRoutingId, 0, sizeof(cRoutingId));
   }
   /** Copy constructor. */
   SIPX_TRANSPORT_DATA(const SIPX_TRANSPORT_DATA& ref)
   {
      copy(ref);
   }
   /** Assignment operator. */
   SIPX_TRANSPORT_DATA& operator=(const SIPX_TRANSPORT_DATA& ref)
   {
      // check for assignment to self
      if (this == &ref) return *this;

      return copy(ref);
   }    

   SIPX_TRANSPORT_DATA& copy(const SIPX_TRANSPORT_DATA& ref)
   {
      hTransport = ref.hTransport;
      pInst = ref.pInst;
      bIsReliable = ref.bIsReliable;
      memset(szTransport, 0, sizeof(szTransport)) ;
      SAFE_STRNCPY(szTransport, ref.szTransport, MAX_TRANSPORT_NAME - 1);
      memset(szLocalIp, 0, sizeof(szLocalIp));
      SAFE_STRNCPY(szLocalIp, ref.szLocalIp, sizeof(szLocalIp)-1);
      memset(cRoutingId, 0, sizeof(cRoutingId));
      SAFE_STRNCPY(cRoutingId, ref.cRoutingId, sizeof(cRoutingId)-1);
      iLocalPort = ref.iLocalPort;
      pFnWriteProc = ref.pFnWriteProc;
      pUserData = ref.pUserData;        
      bRouteByUser = ref.bRouteByUser;
      return *this;
   }

   static const bool isCustomTransport(const SIPX_TRANSPORT_DATA* const pTransport)
   {
      bool bRet = false;
      if (pTransport)
      {
         if (SAFE_STRLEN(pTransport->szTransport) > 0)
         {
            bRet = true;
         }
      }
      return bRet;
   }

   SIPX_TRANSPORT            hTransport;
   SIPX_INSTANCE_DATA*       pInst;
   UtlBoolean                bIsReliable;
   char                      szTransport[MAX_TRANSPORT_NAME];
   char                      szLocalIp[32];
   int                       iLocalPort;
   SIPX_TRANSPORT_WRITE_PROC pFnWriteProc;
   OsMutex*                  pMutex;
   const void*               pUserData;
   char                      cRoutingId[64];
   UtlBoolean                bRouteByUser;
};

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/**
 * Looks up the SIPX_TRANSPORT_DATA structure pointer, given the SIPX_TRANSPORT handle.
 * @param hTransport Transport Handle
 * @param type Lock type to use during lookup.
 */
SIPX_TRANSPORT_DATA* sipxTransportLookup(const SIPX_TRANSPORT hTransport,
                                         SIPX_LOCK_TYPE type,
                                         const OsStackTraceLogger& oneBackInStack);

/**
 * Unlocks the mutex associated with the TRANSPORT DATA
 * 
 * @param pData pointer to the SIPX_TRANSPORT structure
 * @param type Type of lock (read or write)
 */
void sipxTransportReleaseLock(SIPX_TRANSPORT_DATA* pData,
                              SIPX_LOCK_TYPE type,
                              const OsStackTraceLogger& oneBackInStack);

/**
 * Releases the TRANSPORT handle created sipxConfigExternalTransportAdd
 * Also calls sipxTransportFree.
 *
 * @param hInfo Handle to the Transport object
 */
void sipxTransportObjectFree(SIPX_TRANSPORT hTransport);

/**
 * Destroy all external transports for a given instance
 */
void sipxTransportDestroyAll(const SIPX_INST hInst);

const char* sipxTransportTypeToString(SIPX_TRANSPORT_TYPE type);

#endif // SipXTransport_h__
