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
#include "os/OsSysLog.h"
#ifdef HAVE_SSL
#  include "os/OsSSL.h"
#endif
#include "os/OsNatAgentTask.h"
#include "os/HostAdapterAddress.h"
#include "net/SipUserAgent.h"
#include "net/SdpCodecFactory.h"
#include "net/SipRefreshMgr.h"
#include "cp/CallManager.h"
#include "mi/CpMediaInterfaceFactory.h"
#include "tapi/SipXHandleMap.h"
#include "tapi/sipXtapi.h"
#include "tapi/SipXMessageObserver.h"
#include "tapi/SipXConfig.h"
#include "tapi/SipXCall.h"
#include "tapi/SipXKeepaliveEventListener.h"
#include "tapi/SipXTransport.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES

extern SipXHandleMap gTransportHandleMap;

// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
static bool gbHibernated = false;

// GLOBAL FUNCTIONS

// CHECKED
OsStatus initLogger()
{
   return OsSysLog::initialize(0, // do not cache any log messages in memory
                               "sipXtapi"); // name for messages from this program
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigAllowMethod(const SIPX_INST hInst, const char* method, const bool bAllow)
{
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   SIPX_RESULT res = SIPX_RESULT_FAILURE;

   if (pInst)
   {
      pInst->pSipUserAgent->allowMethod(method, bAllow);
      res = SIPX_RESULT_SUCCESS;
   }
   return res;
}

// CHECKED
void freeAudioCodecs(SIPX_INSTANCE_DATA& pInst)
{
   // Did we previously allocate an audio codecs array and store it in our codec settings?
   if (pInst.audioCodecSetting.bInitialized)
   {
      // Free up the previously allocated codecs and the array
      for (int codecIndex = 0; codecIndex < pInst.audioCodecSetting.numCodecs; codecIndex++)
      {

         delete pInst.audioCodecSetting.sdpCodecArray[codecIndex];
         pInst.audioCodecSetting.sdpCodecArray[codecIndex] = NULL;
      }
      delete[] pInst.audioCodecSetting.sdpCodecArray;
      pInst.audioCodecSetting.sdpCodecArray = NULL;
      pInst.audioCodecSetting.numCodecs = 0;
   }
}

// CHECKED
void freeVideoCodecs(SIPX_INSTANCE_DATA& pInst)
{
   // Did we previously allocate a video codecs array and store it in our codec settings?
   if (pInst.videoCodecSetting.bInitialized)
   {
      // Free up the previuosly allocated codecs and the array
      for (int codecIndex = 0; codecIndex < pInst.videoCodecSetting.numCodecs; codecIndex++)
      {
         if (pInst.videoCodecSetting.sdpCodecArray[codecIndex])
         {
            delete pInst.videoCodecSetting.sdpCodecArray[codecIndex];
            pInst.videoCodecSetting.sdpCodecArray[codecIndex] = NULL;
         }
      }
      delete[] pInst.videoCodecSetting.sdpCodecArray;
      pInst.videoCodecSetting.sdpCodecArray = NULL;
      pInst.videoCodecSetting.numCodecs = 0;
   }
}

/***************************************************************************
* Public Config Functions
***************************************************************************/

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigInitLogging(const char* szFilename, SIPX_LOG_LEVEL logLevel) 
{
   SIPX_RESULT res = SIPX_RESULT_FAILURE;

#ifdef ENABLE_LOGGING
   // Start up logger thread
   if (initLogger() == OS_SUCCESS)
   {
      if (OsSysLog::setLoggingPriority((OsSysLogPriority)logLevel) == OS_SUCCESS)
      {
         if (OsSysLog::setOutputFile(0, szFilename) == OS_SUCCESS)
         {
            res = SIPX_RESULT_SUCCESS;
         }
      }
   }
#endif 

   return res;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetLogLevel(SIPX_LOG_LEVEL logLevel) 
{
   SIPX_RESULT res = SIPX_RESULT_FAILURE;

#ifdef ENABLE_LOGGING
   if (OsSysLog::setLoggingPriority((OsSysLogPriority)logLevel) == OS_SUCCESS)
   {
      res = SIPX_RESULT_SUCCESS;
   }
#endif    

   return res;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetLogFile(const char* szFilename)
{
   SIPX_RESULT res = SIPX_RESULT_FAILURE;

#ifdef ENABLE_LOGGING
   if (OsSysLog::setOutputFile(0, szFilename) == OS_SUCCESS)
   {
      res = SIPX_RESULT_SUCCESS;
   }
#endif   
   return res;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetLogCallback(sipxLogCallback pCallback)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetLogCallback");

#ifdef ENABLE_LOGGING
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   if (OsSysLog::setCallbackFunction(pCallback) == OS_SUCCESS)
   {
      rc = SIPX_RESULT_SUCCESS;
   }
   return rc;
#else
   return SIPX_RESULT_SUCCESS;
#endif        
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetOutboundProxy(const SIPX_INST hInst,
                                                    const char* szProxy)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetOutboundProxy");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetOutboundProxy hInst=%p proxy=%s",
      hInst, szProxy);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      assert(pInst->pSipUserAgent);

      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setProxyServers(szProxy);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetDnsSrvTimeouts(const int initialTimeoutInSecs,
                                                     const int retries)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetDnsSrvTimeouts");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetDnsSrvTimeouts initialTimeoutInSecs=%d retries=%d",
      initialTimeoutInSecs, retries);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   assert(initialTimeoutInSecs > 0);
   assert(retries > 0);

   if (initialTimeoutInSecs > 0 && retries > 0)
   {
      SipSrvLookup::setDnsSrvTimeouts(initialTimeoutInSecs, retries);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetRegisterResponseWaitSeconds(const SIPX_INST hInst,
                                                                  const int seconds)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetRegisterResponseWaitSeconds");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetRegisterResponseWaitSeconds hInst=%p seconds=%d",
      hInst, seconds);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      assert(pInst->pSipUserAgent);
      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setRegisterResponseTimeout(seconds);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableRport(const SIPX_INST hInst,
                                               const int bEnable)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableRport");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableRport hInst=%p bEnable=%d",
      hInst, bEnable);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      assert(pInst->pSipUserAgent);
      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setUseRport(bEnable);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetUserAgentName(const SIPX_INST hInst,
                                                    const char* szName,
                                                    const int bIncludePlatform)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetUserAgentName");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetUserAgentName hInst=%p szName=%s",
      hInst, szName);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      assert(pInst->pSipUserAgent);
      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setIncludePlatformInUserAgentName(bIncludePlatform);
         pInst->pSipUserAgent->setUserAgentName(szName);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetRegisterExpiration(const SIPX_INST hInst,
                                                         const int nRegisterExpirationSecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetRegisterExpiration");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetRegisterExpiration hInst=%p seconds=%d",
      hInst, nRegisterExpirationSecs);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      assert(pInst->pRefreshManager);
      if (pInst->pRefreshManager)
      {
         pInst->pRefreshManager->setRegistryPeriod(nRegisterExpirationSecs);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetDnsSrvFailoverTimeout(const SIPX_INST hInst,
                                                            const int failoverTimeoutInSecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetDnsSrvFailoverTimeout");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetDnsSrvFailoverTimeout hInst=%p seconds=%d",
      hInst, failoverTimeoutInSecs);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      assert(pInst->pSipUserAgent);
      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setDnsSrvTimeout(failoverTimeoutInSecs);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableStun(const SIPX_INST hInst,
                                              const char* szServer,
                                              int iServerPort,
                                              int iKeepAliveInSecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableStun");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableStun hInst=%p server=%s:%d keepalive=%d",
      hInst, szServer, iServerPort, iKeepAliveInSecs);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      if (!pInst->pStunNotification)
      {
         pInst->pStunNotification = new OsQueuedEvent(*pInst->pMessageObserver->getMessageQueue(),
                                                      SIPXMO_NOTIFICATION_STUN);
      }

      pInst->pCallManager->enableStun(szServer, iServerPort, iKeepAliveInSecs, pInst->pStunNotification);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigDisableStun(const SIPX_INST hInst)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigDisableStun");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigDisableStun hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->pCallManager->enableStun(NULL, PORT_NONE, 0);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableTurn(const SIPX_INST hInst,
                                              const char*     szServer,
                                              const int       iServerPort,
                                              const char*     szUsername,
                                              const char*     szPassword,
                                              const int       iKeepAliveInSecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableTurn");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableTurn hInst=%p server=%s:%d keepalive=%d",
      hInst, szServer, iServerPort, iKeepAliveInSecs);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->pCallManager->enableTurn(szServer, iServerPort, szUsername,
         szPassword, iKeepAliveInSecs);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigDisableTurn(const SIPX_INST hInst) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigDisableTurn");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigDisableTurn hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->pCallManager->enableTurn(NULL, PORT_NONE, NULL, NULL, 0);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableIce(const SIPX_INST hInst)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableIce");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableICE hInst=%p", hInst);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->pCallManager->enableIce(true);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigDisableIce(const SIPX_INST hInst) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigDisableIce");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigDisableICE hInst=%p", hInst);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->pCallManager->enableIce(false);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigKeepAliveAdd(const SIPX_INST     hInst,
                                                SIPX_CONTACT_ID     contactId,
                                                SIPX_KEEPALIVE_TYPE type,
                                                const char*         remoteIp,
                                                int                 remotePort,
                                                int                 keepAliveSecs) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigKeepAliveAdd");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigKeepAliveAdd hInst=%p type=%d target=%s:%d keepAlive=%d",
      hInst,
      type,
      remoteIp ? remoteIp : "<NULL>",
      remotePort,
      keepAliveSecs);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   UtlString localSocket = "0.0.0.0";
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(pInst);
   assert(remoteIp);
   assert(remotePort > 0);

   if (pInst && remoteIp && remotePort > 0 &&
       type >= SIPX_KEEPALIVE_CRLF && type <= SIPX_KEEPALIVE_SIP_OPTIONS &&
       keepAliveSecs > -2)
   {
      if (contactId > 0)
      {
         SIPX_CONTACT_ADDRESS* pContact = NULL;

         pContact = pInst->pSipUserAgent->getContactDb().getLocalContact(contactId);
         if (pContact)
         {
            // set IP address to that of contact
            localSocket = pContact->cIpAddress;
         }
      }

      switch (type)
      {
      case SIPX_KEEPALIVE_CRLF:
         // connect and send CRLF
         if (pInst->pSipUserAgent->addCrLfKeepAlive(localSocket, 
                  remoteIp, remotePort, keepAliveSecs, pInst->pKeepaliveDispatcher))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            rc = SIPX_RESULT_FAILURE;
         }
         break;
      case SIPX_KEEPALIVE_STUN:
         if (pInst->pSipUserAgent->addStunKeepAlive(localSocket,
                  remoteIp, remotePort, keepAliveSecs, pInst->pKeepaliveDispatcher))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            rc = SIPX_RESULT_FAILURE;
         }
         break;
      case SIPX_KEEPALIVE_SIP_PING:
         // keep alive by sending SIP PING message
         if (pInst->pSipUserAgent->addSipKeepAlive(localSocket,
                  remoteIp, remotePort, "PING", keepAliveSecs, pInst->pKeepaliveDispatcher))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            rc = SIPX_RESULT_FAILURE;
         }
         break;
      case SIPX_KEEPALIVE_SIP_OPTIONS:
         if (pInst->pSipUserAgent->addSipKeepAlive(localSocket,
                  remoteIp, remotePort, "OPTIONS", keepAliveSecs, pInst->pKeepaliveDispatcher))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            rc = SIPX_RESULT_FAILURE;
         }
         break;
      default:
         break;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigKeepAliveRemove(const SIPX_INST     hInst,
                                                   SIPX_CONTACT_ID     contactId,
                                                   SIPX_KEEPALIVE_TYPE type,
                                                   const char*         remoteIp,
                                                   int                 remotePort) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigKeepAliveRemove");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigKeepAliveRemove hInst=%p target=%s:%d",
      hInst,
      remoteIp ? remoteIp : "<NULL>",
      remotePort);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   UtlString localSocket = "0.0.0.0";
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(pInst);
   assert(remoteIp);
   assert(remotePort > 0);
   assert(type >= SIPX_KEEPALIVE_CRLF && type <= SIPX_KEEPALIVE_SIP_OPTIONS);

   if (pInst && remoteIp && remotePort > 0)
   {
      if (contactId > 0)
      {
         SIPX_CONTACT_ADDRESS* pContact = NULL;
         pContact = pInst->pSipUserAgent->getContactDb().getLocalContact(contactId);

         if (pContact)
         {
            // set IP address to that of contact
            localSocket = pContact->cIpAddress;
         }
      }

      switch (type)
      {
      case SIPX_KEEPALIVE_CRLF:
         if (pInst->pSipUserAgent->removeCrLfKeepAlive(localSocket, 
                  remoteIp, remotePort))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            rc = SIPX_RESULT_FAILURE;
         }
         break;
      case SIPX_KEEPALIVE_STUN:
         if (pInst->pSipUserAgent->removeStunKeepAlive(localSocket,
                  remoteIp, remotePort))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            rc = SIPX_RESULT_FAILURE;
         }
         break;
      case SIPX_KEEPALIVE_SIP_PING:
         if (pInst->pSipUserAgent->removeSipKeepAlive(localSocket,
                  remoteIp, remotePort, "PING"))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            rc = SIPX_RESULT_FAILURE;
         }
         break;
      case SIPX_KEEPALIVE_SIP_OPTIONS:
         if (pInst->pSipUserAgent->removeSipKeepAlive(localSocket,
            remoteIp, remotePort, "OPTIONS"))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            rc = SIPX_RESULT_FAILURE;
         }
         break;
      default:
         break;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetVersion(char* szVersion,
                                              const size_t nBuffer)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetVersion");
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   if (szVersion)
   {
      SNPRINTF(szVersion, nBuffer, SIPXTAPI_VERSION_STRING,
               SIPXTAPI_VERSION, SIPXTAPI_BUILDNUMBER,
               __DATE__);

      rc = SIPX_RESULT_SUCCESS;
   }
   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetLocalSipUdpPort(SIPX_INST hInst,
                                                      int* pPort)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetLocalSipUdpPort");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetLocalSipUdpPort hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst && pPort)
   {
      assert(pInst->pSipUserAgent);
      if (pInst->pSipUserAgent)
      {
         *pPort = pInst->pSipUserAgent->getUdpPort();
         if (portIsValid(*pPort))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetLocalSipTcpPort(SIPX_INST hInst,
                                                      int* pPort)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetLocalSipTcpPort");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetLocalSipTcpPort hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst && pPort)
   {
      assert(pInst->pSipUserAgent);
      if (pInst->pSipUserAgent)
      {
         *pPort = pInst->pSipUserAgent->getTcpPort();
         if (portIsValid(*pPort))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetLocalSipTlsPort(SIPX_INST hInst,
                                                      int* pPort)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetLocalSipTlsPort");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetLocalSipTlsPort hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst && pPort)
   {
      assert(pInst->pSipUserAgent);
      if (pInst->pSipUserAgent)
      {
         *pPort = pInst->pSipUserAgent->getTlsPort();
         if (portIsValid(*pPort))
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableDnsSrv(const int enable)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableDnsSrv");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableDnsSrv bEnable=%d",
      enable);

   // The IgnoreSRV option has the opposite sense of bEnable.
   SipSrvLookup::setOption(SipSrvLookup::OptionCodeIgnoreSRV, !enable);

   return SIPX_RESULT_SUCCESS;
}

// CHECKED
// TODO: for INFO support in the future, we will need to remember setting in SIPX_INSTANCE_DATA
SIPXTAPI_API SIPX_RESULT sipxConfigSetOutboundDTMFMode(const SIPX_INST hInst,
                                                       const SIPX_OUTBOUND_DTMF_MODE mode)
{
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetOutboundDTMFMode hInst=%p bEnable=%d",
      hInst, mode);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         if (pInterface->setOutboundDTMFMode((MEDIA_OUTBOUND_DTMF_MODE)mode))
		 {
			rc = SIPX_RESULT_SUCCESS;
		 }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetOutboundDTMFMode(const SIPX_INST hInst,
                                                       SIPX_OUTBOUND_DTMF_MODE* mode)
{
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
	  {
		 if (pInterface->getOutboundDTMFMode(*(MEDIA_OUTBOUND_DTMF_MODE*)mode) == OS_SUCCESS)
		 {
			 rc = SIPX_RESULT_SUCCESS;
		 }
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetOutboundDTMFMode hInst=%p enabled=%d",
      hInst, *mode);

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableInboundDTMF(const SIPX_INST hInst,
                                                     SIPX_INBOUND_DTMF_MODE mode,
                                                     int bEnable)
{
	OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
		"sipxConfigEnableInboundDTMF hInst=%p bEnable=%d",
		hInst, mode);

	SIPX_RESULT rc = SIPX_RESULT_FAILURE;
	SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
	assert(pInst);

	if (pInst)
	{
		CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

		if (pInterface)
		{
			if (pInterface->enableInboundDTMF((MEDIA_INBOUND_DTMF_MODE)mode, bEnable))
			{
				rc = SIPX_RESULT_SUCCESS;
			}
		}
	}

	return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigIsInboundDTMFEnabled(const SIPX_INST hInst,
                                                        SIPX_INBOUND_DTMF_MODE mode,
                                                        int* bEnabled)
{
	SIPX_RESULT rc = SIPX_RESULT_FAILURE;
	SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
	assert(pInst);

	if (pInst)
	{
		CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

		if (pInterface)
		{
			if (pInterface->isInboundDTMFEnabled((MEDIA_INBOUND_DTMF_MODE)mode, *bEnabled) == OS_SUCCESS)
			{
				rc = SIPX_RESULT_SUCCESS;
			}
		}
	}

	OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
		"sipxConfigIsInboundDTMFEnabled hInst=%p mode=%d enabled=%d",
		hInst, mode, *bEnabled);

	return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableRTCP(const SIPX_INST hInst,
                                              const int bEnable) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableRTCP");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableRTCP hInst=%p enable=%d",
      hInst, bEnable);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface && (pInterface->enableRTCP(bEnable) == OS_SUCCESS))
      {
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetTLSSecurityParameters(SIPX_SSL_CRT_VERIFICATION verificationMode,
                                                            const char* szCApath,
                                                            const char* szCAfile,
                                                            const char* szCertificateFile,
                                                            const char* szPrivateKeyFile,
                                                            const char* szPassword)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetSecurityParameters");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetSecurityParameters verificationMode=%d, szCApath=%s, "
      "szCAfile=%s, szCertificateFile=%s, szPrivateKeyFile=%s",
      szCApath != NULL ? szCApath : "",
      szCAfile != NULL ? szCAfile : "",
      szCertificateFile != NULL ? szCertificateFile : "",
      szPrivateKeyFile != NULL ? szPrivateKeyFile : "");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

#ifdef HAVE_SSL
   OsSSL::setCrtVerificationPolicy((OsSSL::SSL_CRT_VERIFICATION)verificationMode);
   OsSSL::setCApath(szCApath);
   OsSSL::setCAfile(szCAfile);
   OsSSL::setCertificateFile(szCertificateFile);
   OsSSL::setPrivateKeyFile(szPrivateKeyFile);
   OsSSL::setPassword(szPassword);

   OsSSL::SSL_INIT_RESULT sslResult = OsSSL::getInstance()->getInitResult(); // init OpenSSL
   if (sslResult == OsSSL::SSL_INIT_SUCCESS)
   {
      rc = SIPX_RESULT_SUCCESS;
   }
#else
   rc = SIPX_RESULT_NOT_IMPLEMENTED;
#endif

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableSipShortNames(const SIPX_INST hInst,
                                                       const int bEnabled)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableSipShortNames");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableSipShortNames hInst=%p, bEnabled=%d",
      hInst, bEnabled);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->bShortNames = bEnabled;

      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setHeaderOptions(pInst->bAllowHeader, pInst->bDateHeader, pInst->bShortNames, pInst->szAcceptLanguage);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableSipDateHeader(const SIPX_INST hInst, 
                                                       const int bEnabled)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableSipDateHeader");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableSipDateHeader hInst=%p, bEnabled=%d",
      hInst, bEnabled);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->bDateHeader = bEnabled;

      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setHeaderOptions(pInst->bAllowHeader, pInst->bDateHeader, pInst->bShortNames, pInst->szAcceptLanguage);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}                                                       

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableSipAllowHeader(const SIPX_INST hInst, 
                                                        const int bEnabled)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableSipAllowHeader");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableSipAllowHeader hInst=%p, bEnabled=%d",
      hInst, bEnabled);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->bAllowHeader = bEnabled;

      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setHeaderOptions(pInst->bAllowHeader, pInst->bDateHeader, pInst->bShortNames, pInst->szAcceptLanguage);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetSipAcceptLanguage(const SIPX_INST hInst, 
                                                        const char* szLanguage)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetSipAcceptLanguage");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetSipAcceptLanguage hInst=%p, szLanguage=%s",
      hInst, szLanguage);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      SAFE_STRNCPY(pInst->szAcceptLanguage, szLanguage, sizeof(pInst->szAcceptLanguage));

      if (pInst->pSipUserAgent)
      {
         pInst->pSipUserAgent->setHeaderOptions(pInst->bAllowHeader, pInst->bDateHeader, pInst->bShortNames, pInst->szAcceptLanguage);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetLocationHeader(const SIPX_INST hInst,
                                                     const char* szHeader)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetLocationHeader");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetLocationHeader hInst=%p, szHeader=%s",
      hInst, szHeader);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst && szHeader)
   {
      SAFE_STRNCPY(pInst->szLocationHeader, szHeader, sizeof(pInst->szLocationHeader));
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetConnectionIdleTimeout(const SIPX_INST hInst,
                                                            const int idleTimeout)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetConnectionIdleTimeout");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetSetConnectionIdleTimeout hInst=%p, idleTimeout==%d",
      hInst, idleTimeout);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface =
         pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         OsStatus status = pInterface->setConnectionIdleTimeout(idleTimeout);

         if (status == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigExternalTransportAdd(SIPX_INST const hInst,
                                                        SIPX_TRANSPORT* hTransport,
                                                        const int bIsReliable,
                                                        const char* szTransport,
                                                        const char* szLocalIp,
                                                        const int iLocalPort,
                                                        SIPX_TRANSPORT_WRITE_PROC writeProc,
                                                        const char* szLocalRoutingId,
                                                        const void* pUserData)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigExternalTransportAdd");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigExternalTransportAdd hInst=%p, reliable=%d, transport=%s, localIp=%s, localPort=%d, routingId=%s",
      hInst,
      bIsReliable, 
      szTransport ? szTransport : "<NULL>",
      szLocalIp ? szLocalIp : "<NULL>",
      iLocalPort,
      szLocalRoutingId ? szLocalRoutingId : "<NULL>") ;

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);
   assert(szTransport);
   assert(szTransport[0] != '\0');
   assert(writeProc);

   if (pInst)
   {
      SIPX_TRANSPORT_DATA* pData = new SIPX_TRANSPORT_DATA();
      pData->pMutex = new OsMutex(OsMutex::Q_FIFO);
      pData->pMutex->acquire();

      UtlBoolean res = gTransportHandleMap.allocHandle(*hTransport, pData);

      if (res)
      {
         pData->pInst = pInst;
         pData->bIsReliable = bIsReliable;
         SAFE_STRNCPY(pData->szTransport, szTransport, sizeof(pData->szTransport));
         SAFE_STRNCPY(pData->szLocalIp, szLocalIp, sizeof(pData->szLocalIp));
         pData->iLocalPort = iLocalPort;
         pData->pFnWriteProc = writeProc;
         pData->pUserData = pUserData;
         pData->hTransport = *hTransport;
         SAFE_STRNCPY(pData->cRoutingId, szLocalRoutingId, sizeof(pData->cRoutingId));

         pData->pInst->pSipUserAgent->addExternalTransport(pData->szTransport, pData);

         pData->pInst->pSipUserAgent->getContactDb().replicateForTransport(TRANSPORT_UDP,
               TRANSPORT_CUSTOM, szTransport, szLocalRoutingId);

         pData->pMutex->release();
         rc = SIPX_RESULT_SUCCESS;
      }
      else
      {
         // handle allocation failure
         OsSysLog::add(FAC_SIPXTAPI, PRI_ERR,
            "allocHandle failed to allocate a handle");
         delete pData->pMutex;
         delete pData;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigExternalTransportRemove(const SIPX_TRANSPORT hTransport)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigExternalTransportRemove");
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigExternalTransportRemove");

   SIPX_TRANSPORT_DATA* pData = sipxTransportLookup(hTransport, SIPX_LOCK_READ, stackLogger);

   if (pData)
   {
      SIPX_INSTANCE_DATA *pInst = pData->pInst;
      assert(pInst);
      SipUserAgent *pSipUserAgent = pInst->pSipUserAgent;

      if (pInst && pSipUserAgent)
      {
         pSipUserAgent->removeExternalTransport(pData->szTransport, pData);

         sipxTransportReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

         pSipUserAgent->getContactDb().removeForTransport(TRANSPORT_CUSTOM);
      }
      else
      {
         sipxTransportReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
      }

      sipxTransportObjectFree(hTransport);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigExternalTransportRouteByUser(const SIPX_TRANSPORT hTransport,
                                                                int bRouteByUser)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigExternalTransportRouteByUser");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_TRANSPORT_DATA* pTransportData = sipxTransportLookup(hTransport, SIPX_LOCK_WRITE, stackLogger);

   if (pTransportData)
   {
      pTransportData->bRouteByUser = bRouteByUser;
      sipxTransportReleaseLock(pTransportData, SIPX_LOCK_WRITE, stackLogger);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED, not sure it works
SIPXTAPI_API SIPX_RESULT sipxConfigExternalTransportHandleMessage(const SIPX_TRANSPORT hTransport,
                                                                  const char* szSourceIP,
                                                                  const int iSourcePort,
                                                                  const char* szLocalIP,
                                                                  const int iLocalPort,
                                                                  const void* pData,
                                                                  const size_t nData)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigExternalTransportHandleMessage");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SipMessage* message = new SipMessage((const char*)pData, (int)nData);

   message->setFromThisSide(false);

   long epochDate;
   if(!message->getDateField(&epochDate))
   {
      message->setDateField();
   }

   message->setSendProtocol(OsSocket::CUSTOM);
   OsTime time;
   OsDateTime::getCurTimeSinceBoot(time);

   message->setTransportTime(time.seconds());

   // Keep track of where this message came from
   message->setSendAddress(szSourceIP, iSourcePort);

   // Keep track of the interface on which this message was
   // received.
   message->setLocalIp(szLocalIP);

   SIPX_TRANSPORT_DATA* pTransportData = sipxTransportLookup(hTransport, SIPX_LOCK_READ, stackLogger);

   if (pTransportData)
   {
      if (pTransportData->pInst && pTransportData->pInst->pSipUserAgent)
      {
         // have the user-agent dispatch it

         if (OsSysLog::willLog(FAC_SIP_CUSTOM, PRI_DEBUG))
         {
            UtlString data((const char*)pData, nData);
            OsSysLog::add(FAC_SIP_CUSTOM, PRI_DEBUG, "[Received] From: %s To: %s \r\n%s\r\n",
               szSourceIP, szLocalIP, data.data());
         }

         pTransportData->pInst->pSipUserAgent->dispatch(message, SipMessageEvent::APPLICATION, pTransportData);
         rc = SIPX_RESULT_SUCCESS;
      }
      sipxTransportReleaseLock(pTransportData, SIPX_LOCK_READ, stackLogger);
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVoiceQualityServer(const SIPX_INST hInst,
                                                         const char* szServer)
{
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVoiceQualityServer Inst=%p target=%s", hInst, szServer);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      if (pInst->pCallManager)
      {
         pInst->pCallManager->setVoiceQualityReportTarget(szServer);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigPrepareToHibernate(const SIPX_INST hInst)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigPrepareToHibernate");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigPrepareToHibernate Inst=%p", hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;


   /*
   // As if 2006-10-06 -- The timer subsystem has been rewritten and no 
   // longer supports the ability to restart timers -- work is needed
   // to add this (not sure how much), but until then, disabling this
   // functionality.

   OsTimer* pTimer;

   if (!gbHibernated)
   {
   gbHibernated = true;
   // hibernate singleton object timers
   // log file timer
   pTimer = OsSysLog::getTimer();
   if (pTimer)
   {
   pTimer->stop();
   }

   pTimer = OsNatAgentTask::getInstance()->getTimer();
   if (pTimer)
   {
   pTimer->stop();
   }
   }

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*) hInst ;  

   if (pInst)
   {   
   pTimer = pInst->pRefreshManager->getTimer();
   if (pTimer)
   {
   pTimer->stop();
   }
   pTimer = pInst->pSipUserAgent->getTimer();
   if (pTimer)
   {
   pTimer->stop();        
   }
   pInst->pSipUserAgent->stopTransactionTimers();

   rc = SIPX_RESULT_SUCCESS;
   }
   */
   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigUnHibernate(const SIPX_INST hInst)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigUnHibernate");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigUnHibernate Inst=%p", hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   /*
   // As if 2006-10-06 -- The timer subsystem has been rewritten and no 
   // longer supports the ability to restart timers -- work is needed
   // to add this (not sure how much), but until then, disabling this
   // functionality.

   OsTimer* pTimer;       
   if (gbHibernated)
   {
   gbHibernated = false;

   // UnHibernate singleton objects


   // log file timer
   pTimer = OsSysLog::getTimer();
   if (pTimer)
   {
   pTimer->start();        
   }

   pTimer = OsNatAgentTask::getInstance()->getTimer();
   if (pTimer)
   {
   pTimer->start();        
   }
   }
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*) hInst ;  

   if (pInst)
   {   
   pTimer = pInst->pRefreshManager->getTimer();
   if (pTimer)
   {
   pTimer->start();        
   }
   pTimer = pInst->pSipUserAgent->getTimer();
   if (pTimer)
   {
   pTimer->start();        
   }
   pInst->pSipUserAgent->startTransactionTimers();

   rc = SIPX_RESULT_SUCCESS;
   }

   */
   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigEnableRtpOverTcp(const SIPX_INST hInst,
                                                    int bEnable)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigEnableRtpOverTcp");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigEnableRtpOverTcp Inst=%p bEnable=%d", hInst, bEnable);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst)
   {
      pInst->bRtpOverTcp = bEnable;
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetAudioCodecPreferences(const SIPX_INST hInst, 
                                                            const SIPX_AUDIO_BANDWIDTH_ID bandWidth)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetAudioCodecPreferences");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetAudioCodecPreferences hInst=%p bandWidth=%d",
      hInst, bandWidth);

   if (pInst)
   {
      // Check if bandwidth is legal, do not allow variable bandwidth
      if (bandWidth >= AUDIO_CODEC_BW_LOW && bandWidth <= AUDIO_CODEC_BW_HIGH)
      {
         CpMediaInterfaceFactory* pInterface = 
            pInst->pCallManager->getMediaInterfaceFactory();

         if (pInterface)
         {
            int numCodecs;
            SdpCodec** codecsArray = NULL;
            UtlString codecName;
            int iRejected;

            pInst->audioCodecSetting.preferences = "";
            int codecIndex;

            /* Unconditionally rebuild codec factory with all supported codecs. If we 
            * don't do this first then only the previously preferred codecs will be used to
            * build the new factory -> that doesn't work for changing from a lower bandwidth to
            * a higher bandwidth.
            */
            pInterface->buildCodecFactory(pInst->pCodecFactory, 
               pInst->audioCodecSetting.preferences, // No audio preferences
               pInst->videoCodecSetting.preferences, // Keep video prefs
               -1, // Allow all formats
               &iRejected);

            // Now pick preferences out of all available codecs
            pInst->pCodecFactory->getCodecs(numCodecs, codecsArray, "audio");

            OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG,
               "sipxConfigSetAudioCodecPreferences number of Codec = %d for hInst=%p",
               numCodecs, hInst);

            // pick codecs which have smaller bandwidth, DTMF has low bandwidth
            for (int i = 0; i < numCodecs; i++)
            {
               if (codecsArray[i]->getBWCost() <= bandWidth)
               {
                  if (pInterface->getCodecNameByType(codecsArray[i]->getCodecType(), codecName) == OS_SUCCESS)
                  {
                     pInst->audioCodecSetting.preferences += " " + codecName;
                  }
               }
            }
            // print selected codecs to logfile
            OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG,
               "sipxConfigSetAudioCodecPreferences: %s", pInst->audioCodecSetting.preferences.data());

            // select codecs by name
            SIPX_RESULT res = sipxConfigSetAudioCodecByName(hInst, pInst->audioCodecSetting.preferences);

            // override bandwidth, as it was set to AUDIO_CODEC_BW_CUSTOM
            pInst->audioCodecSetting.codecPref = bandWidth;

            if (res == SIPX_RESULT_SUCCESS)
            {
               rc = SIPX_RESULT_SUCCESS;
            }

            // Free up the codecs and the array
            for (codecIndex = 0; codecIndex < numCodecs; codecIndex++)
            {
               delete codecsArray[codecIndex];
               codecsArray[codecIndex] = NULL;
            }
            delete[] codecsArray;
            codecsArray = NULL;
         }
      }
      else
      {
         rc = SIPX_RESULT_INVALID_ARGS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetAudioCodecByName(const SIPX_INST hInst, 
                                                       const char* szCodecName)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetAudioCodecByName");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetAudioCodecByName hInst=%p codec=%s",
      hInst, szCodecName);

   if (pInst)
   {
      int iRejected;

      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         pInst->audioCodecSetting.preferences = szCodecName;

         // maybe add " audio/telephone-event"
         if (!pInst->audioCodecSetting.preferences.contains("audio/telephone-event"))
         {
            pInst->audioCodecSetting.preferences += " audio/telephone-event";
         }

         if (pInst->audioCodecSetting.preferences.length() != 0)
         {
            freeAudioCodecs(*pInst);

            pInterface->buildCodecFactory(pInst->pCodecFactory,
                  pInst->audioCodecSetting.preferences,
                  pInst->videoCodecSetting.preferences,
                  pInst->videoCodecSetting.videoFormat, // Allow all formats
                  &iRejected);

            // We've rebuilt the factory, so get the new count of codecs
            pInst->pCodecFactory->getCodecs(pInst->audioCodecSetting.numCodecs,
                  pInst->audioCodecSetting.sdpCodecArray,
                  "audio");

            pInst->audioCodecSetting.codecPref = AUDIO_CODEC_BW_CUSTOM;

            if (pInst->audioCodecSetting.numCodecs > 1)
            {
               rc = SIPX_RESULT_SUCCESS;
            }
            else
            {
               OsSysLog::add(FAC_SIPXTAPI, PRI_ERR,
                  "sipxConfigSetAudioCodecByName: Setting %s failed, only DTMF is active", 
                  szCodecName);
            }

            pInst->audioCodecSetting.bInitialized = true;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetAudioCodecPreferences(const SIPX_INST hInst,
                                                            SIPX_AUDIO_BANDWIDTH_ID *pBandWidth)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetAudioCodecPreferences");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pInst->audioCodecSetting.bInitialized)
   {
      *pBandWidth = pInst->audioCodecSetting.codecPref;
      rc = SIPX_RESULT_SUCCESS;
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetAudioCodecPreferences hInst=%p bandWidth=%d",
      hInst, *pBandWidth);

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetNumAudioCodecs(const SIPX_INST hInst, 
                                                     int* pNumCodecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetNumAudioCodecs");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pNumCodecs)
   {
      assert(pInst->audioCodecSetting.bInitialized);

      if (pInst->audioCodecSetting.bInitialized)
      {
         *pNumCodecs = pInst->audioCodecSetting.numCodecs;
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetNumAudioCodecs hInst=%p numCodecs=%d",
      hInst, *pNumCodecs);

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetAudioCodec(const SIPX_INST hInst,
                                                 const int index,
                                                 SIPX_AUDIO_CODEC* pCodec)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetAudioCodec");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   UtlString codecName;

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pCodec);

   if (pInst && pCodec && pInst->audioCodecSetting.bInitialized)
   {
      memset((void*)pCodec, 0, sizeof(SIPX_AUDIO_CODEC));

      if (index >= 0 && index < pInst->audioCodecSetting.numCodecs)
      {
         CpMediaInterfaceFactory* pInterface = 
            pInst->pCallManager->getMediaInterfaceFactory();

         // If a name is found for the codec type, copy name and bandwidth cost
         if (pInterface->getCodecNameByType(pInst->audioCodecSetting.sdpCodecArray[index]->getCodecType(),
            codecName))
         {
            SAFE_STRNCPY(pCodec->cName, codecName, SIPXTAPI_CODEC_NAMELEN);
            pCodec->iBandWidth = 
               (SIPX_AUDIO_BANDWIDTH_ID)pInst->audioCodecSetting.sdpCodecArray[index]->getBWCost();
            pCodec->iPayloadType = pInst->audioCodecSetting.sdpCodecArray[index]->getCodecPayloadFormat();

            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetAudioCodec hInst=%p index=%d, codec-%s",
      hInst, index, codecName.data());

   return rc;
}

/***************************************************************************
* Public Video Config Functions
***************************************************************************/

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetVideoCodecPreferences(const SIPX_INST hInst,
                                                            SIPX_VIDEO_BANDWIDTH_ID *pBandWidth)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetVideoCodecPreferences");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pInst->videoCodecSetting.bInitialized)
   {
      *pBandWidth = pInst->videoCodecSetting.codecPref;
      rc = SIPX_RESULT_SUCCESS;
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetVideoCodecPreferences hInst=%p bandWidth=%d",
      hInst, *pBandWidth);

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED, doesn't work like for audio codecs, just sets bitrate & framerate
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoBandwidth(const SIPX_INST hInst,
                                                     SIPX_VIDEO_BANDWIDTH_ID bandWidth)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoBandwidth");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoBandwidth hInst=%p bandWidth=%d",
      hInst, bandWidth);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      if (!pInst->videoCodecSetting.bInitialized)
      {
         sipxConfigResetVideoCodecs(hInst);
      }

      // Check if bandwidth is legal, do not allow variable bandwidth
      if (bandWidth >= VIDEO_CODEC_BW_LOW && bandWidth <= VIDEO_CODEC_BW_HIGH)
      {
         CpMediaInterfaceFactory* pImpl = 
            pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

         if (pImpl)
         {
            int frameRate;
            pImpl->getVideoFrameRate(frameRate);

            switch (bandWidth)
            {
            case VIDEO_CODEC_BW_LOW:
               sipxConfigSetVideoParameters(hInst, 5, 10);
               break;
            case VIDEO_CODEC_BW_NORMAL:
               sipxConfigSetVideoParameters(hInst, 70, frameRate);
               break;
            case VIDEO_CODEC_BW_HIGH:
               sipxConfigSetVideoParameters(hInst, 400, frameRate);
               break;
            }
            rc = SIPX_RESULT_SUCCESS;
         }         
      }
   }

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetVideoCaptureDevices(const SIPX_INST hInst,
                                                          char** arrSzCaptureDevices,
                                                          int nDeviceStringLength,
                                                          int nArrayLength)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetVideoCaptureDevices");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetVideoCaptureDevices hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   char *pTemp = (char*)arrSzCaptureDevices;
   memset(arrSzCaptureDevices, 0, nDeviceStringLength * nArrayLength);

   if (pInst && pInst->pCallManager)
   {
      CpMediaInterfaceFactory* pImpl = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         UtlSList captureDevices;

         if (pImpl->getVideoCaptureDevices(captureDevices) == OS_SUCCESS)
         {
            UtlSListIterator iterator(captureDevices);
            UtlString* pDevice;
            int index = 0;
            int bytesToCopy;

            while (pDevice = dynamic_cast<UtlString*>(iterator()))
            {
               if (pDevice->length() > 0)
               {
                  if (pDevice->length() > nDeviceStringLength)
                  {
                     bytesToCopy = nDeviceStringLength;
                  }
                  else
                  {
                     bytesToCopy = pDevice->length();
                  }

                  SAFE_STRNCPY(pTemp, pDevice->data(), bytesToCopy);

                  index++;
                  pTemp += nDeviceStringLength;
                  if (index >= nArrayLength)
                  {
                     break;
                  }
               }
            }
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}                                                          

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetVideoCaptureDevice(const SIPX_INST hInst,
                                                         char* szCaptureDevice,
                                                         int nLength)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetVideoCaptureDevice");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetVideoCaptureDevice hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pInst->pCallManager)
   {
      CpMediaInterfaceFactory* pImpl =
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         UtlString captureDevice;

         if (pImpl->getVideoCaptureDevice(captureDevice) == OS_SUCCESS)
         {
            int bytesToCopy;

            if (captureDevice.length() > nLength)
            {
               bytesToCopy = nLength;
            }
            else
            {
               bytesToCopy = captureDevice.length();
            }

            SAFE_STRNCPY(szCaptureDevice, captureDevice.data(), bytesToCopy);

            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoCaptureDevice(const SIPX_INST hInst,
                                                         const char* szCaptureDevice)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoCaptureDevice");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoCaptureDevice hInst=%p device=%s",
      hInst, szCaptureDevice);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pInst->pCallManager)
   {
      CpMediaInterfaceFactory* pImpl = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         UtlString captureDevice(szCaptureDevice);

         if (pImpl->setVideoCaptureDevice(captureDevice) == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetNumVideoCodecs(const SIPX_INST hInst,
                                                     int* pNumCodecs)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetNumVideoCodecs");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pNumCodecs)
   {
      assert(pInst->videoCodecSetting.bInitialized);

      if (pInst->videoCodecSetting.bInitialized)
      {
         *pNumCodecs = pInst->videoCodecSetting.numCodecs;
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetNumVideoCodecs hInst=%p numCodecs=%d",
      hInst, *pNumCodecs);

   return rc;
#else
   if (pNumCodecs)
   {
      *pNumCodecs = 0;
   }
   return SIPX_RESULT_SUCCESS;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetVideoCodec(const SIPX_INST hInst, 
                                                 const int index, 
                                                 SIPX_VIDEO_CODEC* pCodec)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetVideoCodec");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   UtlString codecName;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pCodec);

   if (pInst && pCodec && pInst->videoCodecSetting.bInitialized)
   {
      memset((void*)pCodec, 0, sizeof(SIPX_VIDEO_CODEC));

      if (index >= 0 && index < pInst->videoCodecSetting.numCodecs)
      {
         CpMediaInterfaceFactory* pInterface = 
            pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

         // If a name is found for the codec type, copy name and bandwidth cost
         if (pInterface->getCodecNameByType(pInst->videoCodecSetting.sdpCodecArray[index]->getCodecType(),
            codecName))
         {
            SAFE_STRNCPY(pCodec->cName, codecName, SIPXTAPI_CODEC_NAMELEN);
            pCodec->iBandWidth = 
               (SIPX_VIDEO_BANDWIDTH_ID)pInst->videoCodecSetting.sdpCodecArray[index]->getBWCost();
            pCodec->iPayloadType = pInst->videoCodecSetting.sdpCodecArray[index]->getCodecPayloadFormat();

            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetVideoCodec hInst=%p index=%d, codec-%s",
      hInst, index, codecName.data());

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoCodecByName(const SIPX_INST hInst, 
                                                       const char* szCodecName)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoCodecByName");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoCodecByName hInst=%p codec=%s",
      hInst, szCodecName);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      int iRejected;

      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pInterface)
      {
         freeVideoCodecs(*pInst);

         pInst->videoCodecSetting.preferences = szCodecName;

         pInterface->buildCodecFactory(pInst->pCodecFactory, 
               pInst->audioCodecSetting.preferences,
               pInst->videoCodecSetting.preferences,
               pInst->videoCodecSetting.videoFormat, // Allow all formats
               &iRejected);

         // We've rebuilt the factory, so get the new count of codecs
         pInst->pCodecFactory->getCodecs(pInst->videoCodecSetting.numCodecs,
               pInst->videoCodecSetting.sdpCodecArray,
               "video");

         if (pInst->videoCodecSetting.numCodecs > 0)
         {
            pInst->videoCodecSetting.codecPref = VIDEO_CODEC_BW_CUSTOM;
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            // Codec setting by name failed - we have an empty codec factory.
            // Fall back to previously set bandwidth Id
            OsSysLog::add(FAC_SIPXTAPI, PRI_ERR,
               "sipxConfigSetVideoCodecByName: Setting %s failed", 
               szCodecName);
         }
         pInst->videoCodecSetting.bInitialized = true;
      }
   }

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED, problem pInst->videoCodecSetting.pPreferences will be empty after execution
SIPXTAPI_API SIPX_RESULT sipxConfigResetVideoCodecs(const SIPX_INST hInst)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigResetVideoCodecs");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigResetVideoCodecs hInst=%p", hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   // passing zero string resets codecs
   rc = sipxConfigSetVideoCodecByName(hInst, "");

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoFormat(const SIPX_INST hInst,
                                                  SIPX_VIDEO_FORMAT videoFormat)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoFormat");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoFormat hInst=%p videoFormat=%d", hInst, videoFormat);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      pInst->videoCodecSetting.videoFormat = videoFormat;

      if (pInst->videoCodecSetting.bInitialized)
      {
         rc = sipxConfigSetVideoCodecByName(hInst, pInst->videoCodecSetting.preferences);
      }
      else
      {
         rc = sipxConfigResetVideoCodecs(hInst);
      }
   }

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoPreviewDisplay(const SIPX_INST hInst,
                                                          SIPX_VIDEO_DISPLAY* const pDisplay)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoPreviewDisplay");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoPreviewWindow hInst=%p, hDisplay=%p",
      hInst, pDisplay);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pImpl = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         if (pImpl->setVideoPreviewDisplay(pDisplay) == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigUpdatePreviewWindow(const SIPX_INST hInst,
                                                       const SIPX_WINDOW_HANDLE hWnd)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigUpdatePreviewWindow");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigUpdatePreviewWindow hInst=%p, hWnd=%p",
      hInst, hWnd);

   return SIPX_RESULT_NOT_IMPLEMENTED;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif

}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxCallResizeWindow(const SIPX_CALL hCall,
                                              const SIPX_WINDOW_HANDLE hWnd)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallResizeWindow");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigResizeWindow hCall=%d, hWnd=%p",
      hCall, hWnd);

   return SIPX_RESULT_NOT_IMPLEMENTED;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif

}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxCallUpdateVideoWindow(const SIPX_CALL hCall,
                                                   const SIPX_WINDOW_HANDLE hWnd)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallUpdateVideoWindow");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxCallUpdateVideoWindow hCall=%d, hWnd=%p",
      hCall, hWnd);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst;
   UtlString sessionCallId;
   UtlString remoteAddress;

   if (sipxCallGetCommonData(hCall, &pInst, NULL, &sessionCallId, &remoteAddress, NULL))
   {
      // for now, just call the sipxConfigUpdatePreviewWindow - it does the same thing
      sr = sipxConfigUpdatePreviewWindow(pInst, hWnd);
   }

   return sr;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoQuality(const SIPX_INST hInst,
                                                   const SIPX_VIDEO_QUALITY_ID quality)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoQuality");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoQuality hInst=%p, quality=%d",
      hInst, quality);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (quality >= VIDEO_QUALITY_LOW && quality <= VIDEO_QUALITY_HIGH)
   {
      SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

      CpMediaInterfaceFactory* pImpl =
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         if (pImpl->setVideoQuality(quality) == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }      
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif

}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoParameters(const SIPX_INST hInst,
                                                      const int bitRate,
                                                      const int frameRate)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoParameters");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoParameters hInst=%p, bitRate=%d, frameRate=%d",
      hInst, bitRate, frameRate);

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   int localBitrate = bitRate;

   if (pInst)
   {
      if (localBitrate < MIN_VIDEO_BITRATE)
      {
         OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
            "sipxConfigSetVideoParameters - Setting localBitrate to 5");
         localBitrate = MIN_VIDEO_BITRATE;
      }
      else if (localBitrate > MAX_VIDEO_BITRATE)
      {
         OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
            "sipxConfigSetVideoParameters - Setting localBitrate to 400");
         localBitrate = MAX_VIDEO_BITRATE;
      }

      CpMediaInterfaceFactory* pImpl = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         if (pImpl->setVideoParameters(localBitrate, frameRate) == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }      
   }
   
   return sr;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoCpuUsage(const SIPX_INST hInst,
                                                    const int cpuUsage)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoCpuUsage");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoCpuUsage hInst=%p, cpuUsage=%d",
      hInst, cpuUsage);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pImpl =
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         if (pImpl->setVideoCpuValue(cpuUsage) == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoBitrate(const SIPX_INST hInst,
                                                   const int bitRate)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoBitrate");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoBitrate hInst=%d, bitRate=%d",
      hInst, bitRate);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pImpl =
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         int localBitrate = bitRate;

         if (localBitrate < MIN_VIDEO_BITRATE)
         {
            OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
               "sipxConfigSetVideoBitrate - Setting localBitrate to 5");
            localBitrate = MIN_VIDEO_BITRATE;
         }
         else if (localBitrate > MAX_VIDEO_BITRATE)
         {
            OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
               "sipxConfigSetVideoBitrate - Setting localBitrate to 400");
            localBitrate = MAX_VIDEO_BITRATE;
         }

         if (pImpl->setVideoBitrate(localBitrate) == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoFramerate(const SIPX_INST hInst,
                                                     const int frameRate)
{
#ifdef VIDEO
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoFramerate");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoFramerate hInst=%d, frameRate=%d",
      hInst, frameRate);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pImpl =
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pImpl)
      {
         if (pImpl->setVideoFramerate(frameRate) == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
#else
   return SIPX_RESULT_NOT_SUPPORTED;
#endif
}

/***************************************************************************
* Public Misc Config Functions
***************************************************************************/

// CHECKED, verify that cbsize of addresses is correct
SIPXTAPI_API SIPX_RESULT sipxConfigGetLocalContacts(const SIPX_INST hInst,
                                                    SIPX_CONTACT_ADDRESS addresses[],
                                                    size_t nMaxAddresses,
                                                    size_t* nActualAddresses) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetLocalContacts");

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   UtlString address;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(pInst);
   assert(pInst->pSipUserAgent);
   *nActualAddresses = 0;

   if (pInst && pInst->pSipUserAgent && nMaxAddresses > 0)
   {
      SIPX_CONTACT_ADDRESS* contacts[MAX_IP_ADDRESSES];
      int numContacts = 0;
      pInst->pSipUserAgent->getContactAddresses(contacts, numContacts);       

      // copy contact records
      for (unsigned int i = 0; (i < (unsigned int)numContacts) && (i < nMaxAddresses); i++)
      {
         addresses[i] = *contacts[i];
         if (addresses[i].eTransportType > TRANSPORT_CUSTOM)
         {
            addresses[i].eTransportType = TRANSPORT_CUSTOM;
         }

         OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
            "sipxConfigGetLocalContacts index=%d contactId=%d contactType=%s transportType=%s port=%d address=%s adapter=%s",
            i,
            contacts[i]->id,
            sipxContactTypeToString(contacts[i]->eContactType),
            sipxTransportTypeToString(contacts[i]->eTransportType),
            contacts[i]->iPort,
            contacts[i]->cIpAddress,
            contacts[i]->cInterface);

         (*nActualAddresses)++;
      }

      rc = SIPX_RESULT_SUCCESS;
   }
   else
   {
      rc = SIPX_RESULT_FAILURE;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetLocalFeedbackAddress(const SIPX_INST hInst,
                                                           const char* szRemoteIp,
                                                           const int iRemotePort,
                                                           char* szContactIp,
                                                           size_t nContactIpLength,
                                                           int* iContactPort,
                                                           int iTimeoutMs) 
{
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetLocalFeedbackAddress hInst=%d szRemoteIp=%s iRemotePort=%i",
      hInst, szRemoteIp ? szRemoteIp : "<NULL>", iRemotePort);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && szRemoteIp && (iRemotePort > 0) && szContactIp)
   {
      memset(szContactIp, 0, nContactIpLength);
      *iContactPort = 0;

      UtlString contactAddress;
      int contactPort;

      if (OsNatAgentTask::getInstance()->findContactAddress(
               szRemoteIp, iRemotePort, &contactAddress, &contactPort, iTimeoutMs))
      {
         SAFE_STRNCPY(szContactIp, contactAddress, nContactIpLength);
         *iContactPort = contactPort;
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxConfigGetAllLocalNetworkIps(char* arrAddresses[],
                                                         char* arrAddressAdapter[],
                                                         int* numAddresses)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetAllLocalNetworkIps");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   const HostAdapterAddress* utlAddresses[SIPX_MAX_IP_ADDRESSES];

   if (*numAddresses > SIPX_MAX_IP_ADDRESSES)
   {
      *numAddresses = SIPX_MAX_IP_ADDRESSES;
   }

   if (getAllLocalHostIps(utlAddresses, *numAddresses))
   {
      rc = SIPX_RESULT_SUCCESS;

      for (int i = 0; i < *numAddresses; i++)
      {
         arrAddresses[i] = SAFE_STRDUP(utlAddresses[i]->mAddress);
         arrAddressAdapter[i] = SAFE_STRDUP(utlAddresses[i]->mAdapter);

         OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
            "sipxConfigGetAllLocalNetworkIps index=%d address=%s adapter=%s",
            i, arrAddresses[i], arrAddressAdapter[i]);

         delete utlAddresses[i];
      }
   }
   else
   {
      *numAddresses = 0;
   }

   return rc;
}
