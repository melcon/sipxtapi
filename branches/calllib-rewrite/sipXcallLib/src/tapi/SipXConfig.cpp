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
#include "sdp/SdpCodecList.h"
#include <sdp/SdpCodecFactory.h>
#include "net/SipRefreshMgr.h"
#include "cp/XCpCallManager.h"
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


OsStatus initLogger()
{
   return OsSysLog::initialize(0, // do not cache any log messages in memory
                               "sipXtapi"); // name for messages from this program
}


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

/***************************************************************************
* Public Config Functions
***************************************************************************/


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
         pInst->pSipUserAgent->setDefaultProxyServers(szProxy);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   return rc;
}


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
      pInst->pCallManager->setEnableICE(TRUE);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}


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
      pInst->pCallManager->setEnableICE(FALSE);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}


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
                  remoteIp, remotePort, keepAliveSecs, pInst->pKeepaliveEventListener))
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
                  remoteIp, remotePort, keepAliveSecs, pInst->pKeepaliveEventListener))
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
                  remoteIp, remotePort, SIP_OPTIONS_METHOD, keepAliveSecs, pInst->pKeepaliveEventListener))
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
      case SIPX_KEEPALIVE_SIP_OPTIONS:
         if (pInst->pSipUserAgent->removeSipKeepAlive(localSocket,
            remoteIp, remotePort, SIP_OPTIONS_METHOD))
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

SIPXTAPI_API SIPX_RESULT sipxConfigSelectAudioCodecByName(const SIPX_INST hInst, 
                                                          const char* szCodecNames)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSelectAudioCodecByName");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSelectAudioCodecByName hInst=%p codec=%s",
      hInst, szCodecNames);

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterfaceFactory = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterfaceFactory)
      {
         if (szCodecNames)
         {
            // add telephone-event if some codec names were specified
            pInst->selectedAudioCodecNames = SdpCodecFactory::getFixedAudioCodecs(szCodecNames);
         }
         else
         {
            pInst->selectedAudioCodecNames = szCodecNames;
         }

         OsStatus res = pInterfaceFactory->buildCodecList(*pInst->pSelectedCodecList,
               pInst->selectedAudioCodecNames,
               pInst->selectedVideoCodecNames);

         if (res == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            OsSysLog::add(FAC_SIPXTAPI, PRI_ERR, "sipxConfigSelectAudioCodecByName: Setting %s failed", szCodecNames);
         }
      }
   }

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigGetNumSelectedAudioCodecs(const SIPX_INST hInst, 
                                                             int* pNumCodecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetNumSelectedAudioCodecs");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pNumCodecs)
   {
      assert(pInst->pSelectedCodecList);

      if (pInst->pSelectedCodecList)
      {
         *pNumCodecs = pInst->pSelectedCodecList->getCodecCount(MIME_TYPE_AUDIO);
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetNumSelectedAudioCodecs hInst=%p numCodecs=%d",
      hInst, *pNumCodecs);

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigGetNumAvailableAudioCodecs(const SIPX_INST hInst, 
                                                              int* pNumCodecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetNumAvailableAudioCodecs");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pNumCodecs)
   {
      *pNumCodecs = pInst->pAvailableCodecList->getCodecCount(MIME_TYPE_AUDIO);
      rc = SIPX_RESULT_SUCCESS;
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetNumAvailableAudioCodecs hInst=%p numCodecs=%d",
      hInst, *pNumCodecs);

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigGetSelectedAudioCodec(const SIPX_INST hInst,
                                                         const int index,
                                                         SIPX_AUDIO_CODEC* pCodec)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetSelectedAudioCodec");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   UtlString codecName;

   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pCodec);

   if (pInst && pCodec)
   {
      memset((void*)pCodec, 0, sizeof(SIPX_AUDIO_CODEC));

      SdpCodec sdpCodec;
      UtlBoolean bFound = pInst->pSelectedCodecList->getCodecByIndex(MIME_TYPE_AUDIO, index, sdpCodec);
      if (bFound)
      {
         SAFE_STRNCPY(pCodec->cName, sdpCodec.getCodecName().data(), SIPXTAPI_CODEC_NAMELEN);
         pCodec->iBandWidth = (SIPX_AUDIO_BANDWIDTH_ID)sdpCodec.getBWCost();
         pCodec->iPayloadType = sdpCodec.getCodecPayloadId();
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetSelectedAudioCodec hInst=%p index=%d, codec-%s",
      hInst, index, codecName.data());

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigGetAvailableAudioCodec(const SIPX_INST hInst,
                                                          const int index,
                                                          SIPX_AUDIO_CODEC* pCodec)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetAvailableAudioCodec");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pCodec);

   if (pInst && pCodec)
   {
      memset((void*)pCodec, 0, sizeof(SIPX_AUDIO_CODEC));

      SdpCodec sdpCodec;
      UtlBoolean bFound = pInst->pAvailableCodecList->getCodecByIndex(MIME_TYPE_AUDIO, index, sdpCodec);
      if (bFound)
      {
         SAFE_STRNCPY(pCodec->cName, sdpCodec.getCodecName().data(), SIPXTAPI_CODEC_NAMELEN);
         pCodec->iBandWidth = (SIPX_AUDIO_BANDWIDTH_ID)sdpCodec.getBWCost();
         pCodec->iPayloadType = sdpCodec.getCodecPayloadId();
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetAvailableAudioCodec hInst=%p index=%d",
      hInst, index);

   return rc;
}

/***************************************************************************
* Public Video Config Functions
***************************************************************************/

SIPXTAPI_API SIPX_RESULT sipxConfigGetVideoCaptureDevices(const SIPX_INST hInst,
                                                          char** arrSzCaptureDevices,
                                                          int nDeviceStringLength,
                                                          int nArrayLength)
{
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
      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

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
                  if (pDevice->length() > (size_t)nDeviceStringLength)
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
}                                                          

SIPXTAPI_API SIPX_RESULT sipxConfigGetVideoCaptureDevice(const SIPX_INST hInst,
                                                         char* szCaptureDevice,
                                                         int nLength)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetVideoCaptureDevice");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetVideoCaptureDevice hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pInst->pCallManager)
   {
      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

      if (pImpl)
      {
         UtlString captureDevice;

         if (pImpl->getVideoCaptureDevice(captureDevice) == OS_SUCCESS)
         {
            int bytesToCopy;

            if (captureDevice.length() > (size_t)nLength)
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
}

SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoCaptureDevice(const SIPX_INST hInst,
                                                         const char* szCaptureDevice)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoCaptureDevice");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoCaptureDevice hInst=%p device=%s",
      hInst, szCaptureDevice);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pInst->pCallManager)
   {
      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

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
}

SIPXTAPI_API SIPX_RESULT sipxConfigGetNumSelectedVideoCodecs(const SIPX_INST hInst,
                                                             int* pNumCodecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetNumSelectedVideoCodecs");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pNumCodecs)
   {
      *pNumCodecs = pInst->pSelectedCodecList->getCodecCount(MIME_TYPE_VIDEO);
      rc = SIPX_RESULT_SUCCESS;
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetNumSelectedVideoCodecs hInst=%p numCodecs=%d",
      hInst, *pNumCodecs);

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigGetNumAvailableVideoCodecs(const SIPX_INST hInst,
                                                              int* pNumCodecs)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetNumAvailableVideoCodecs");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && pNumCodecs)
   {
      *pNumCodecs = pInst->pAvailableCodecList->getCodecCount(MIME_TYPE_VIDEO);
      rc = SIPX_RESULT_SUCCESS;
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetNumAvailableVideoCodecs hInst=%p numCodecs=%d",
      hInst, *pNumCodecs);

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigGetSelectedVideoCodec(const SIPX_INST hInst, 
                                                         const int index, 
                                                         SIPX_VIDEO_CODEC* pCodec)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetSelectedVideoCodec");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   UtlString codecName;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pCodec);

   if (pInst && pCodec)
   {
      memset((void*)pCodec, 0, sizeof(SIPX_VIDEO_CODEC));

      SdpCodec sdpCodec;
      UtlBoolean bFound = pInst->pSelectedCodecList->getCodecByIndex(MIME_TYPE_VIDEO, index, sdpCodec);
      if (bFound)
      {
         SAFE_STRNCPY(pCodec->cName, sdpCodec.getCodecName().data(), SIPXTAPI_CODEC_NAMELEN);
         pCodec->iBandWidth = (SIPX_VIDEO_BANDWIDTH_ID)sdpCodec.getBWCost();
         pCodec->iPayloadType = sdpCodec.getCodecPayloadId();
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetSelectedVideoCodec hInst=%p index=%d, codec-%s",
      hInst, index, codecName.data());

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigGetAvailableVideoCodec(const SIPX_INST hInst, 
                                                          const int index, 
                                                          SIPX_VIDEO_CODEC* pCodec)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigGetAvailableAudioCodec");

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pCodec);

   if (pInst && pCodec)
   {
      memset((void*)pCodec, 0, sizeof(SIPX_VIDEO_CODEC));

      SdpCodec sdpCodec;
      UtlBoolean bFound = pInst->pAvailableCodecList->getCodecByIndex(MIME_TYPE_VIDEO, index, sdpCodec);
      if (bFound)
      {
         SAFE_STRNCPY(pCodec->cName, sdpCodec.getCodecName().data(), SIPXTAPI_CODEC_NAMELEN);
         pCodec->iBandWidth = (SIPX_VIDEO_BANDWIDTH_ID)sdpCodec.getBWCost();
         pCodec->iPayloadType = sdpCodec.getCodecPayloadId();
         rc = SIPX_RESULT_SUCCESS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigGetAvailableAudioCodec hInst=%p index=%d",
      hInst, index);

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigSelectVideoCodecByName(const SIPX_INST hInst, 
                                                          const char* szCodecName)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSelectVideoCodecByName");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSelectVideoCodecByName hInst=%p codec=%s",
      hInst, szCodecName);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         pInst->selectedVideoCodecNames = szCodecName;

         OsStatus res = pInterface->buildCodecList(*pInst->pSelectedCodecList, 
               pInst->selectedAudioCodecNames,
               pInst->selectedVideoCodecNames);

         if (res == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
         else
         {
            // Codec setting by name failed - we have an empty codec factory.
            // Fall back to previously set bandwidth Id
            OsSysLog::add(FAC_SIPXTAPI, PRI_ERR, "sipxConfigSelectVideoCodecByName: Setting %s failed", szCodecName);
         }
      }
   }

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigResetSelectedVideoCodecs(const SIPX_INST hInst)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigResetSelectedVideoCodecs");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigResetSelectedVideoCodecs hInst=%p", hInst);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   // passing zero string resets codecs
   rc = sipxConfigSelectVideoCodecByName(hInst, "");

   return rc;
}


SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoFormat(const SIPX_INST hInst,
                                                  SIPX_VIDEO_FORMAT videoFormat)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoFormat");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoFormat hInst=%p videoFormat=%d", hInst, videoFormat);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      pInst->videoFormat = videoFormat;
      // TODO: rebuild video codecs, with only those video codecs that have given format
      // currently we only support rebuilding codecs by name
   }

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoPreviewDisplay(const SIPX_INST hInst,
                                                          SIPX_VIDEO_DISPLAY* const pDisplay)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoPreviewDisplay");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoPreviewWindow hInst=%p, hDisplay=%p",
      hInst, pDisplay);

   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

      if (pImpl)
      {
         if (pImpl->setVideoPreviewDisplay(pDisplay) == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigUpdatePreviewWindow(const SIPX_INST hInst,
                                                       const SIPX_WINDOW_HANDLE hWnd)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigUpdatePreviewWindow");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigUpdatePreviewWindow hInst=%p, hWnd=%p",
      hInst, hWnd);

   return SIPX_RESULT_NOT_IMPLEMENTED;
}

SIPXTAPI_API SIPX_RESULT sipxCallResizeWindow(const SIPX_CALL hCall,
                                              const SIPX_WINDOW_HANDLE hWnd)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxCallResizeWindow");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigResizeWindow hCall=%d, hWnd=%p",
      hCall, hWnd);

   return SIPX_RESULT_NOT_IMPLEMENTED;
}

SIPXTAPI_API SIPX_RESULT sipxCallUpdateVideoWindow(const SIPX_CALL hCall,
                                                   const SIPX_WINDOW_HANDLE hWnd)
{
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
}

SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoQuality(const SIPX_INST hInst,
                                                   const SIPX_VIDEO_QUALITY_ID quality)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoQuality");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoQuality hInst=%p, quality=%d",
      hInst, quality);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (quality >= VIDEO_QUALITY_LOW && quality <= VIDEO_QUALITY_HIGH)
   {
      SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

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
}

SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoParameters(const SIPX_INST hInst,
                                                      const int bitRate,
                                                      const int frameRate)
{
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

      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

      if (pImpl)
      {
         if (pImpl->setVideoParameters(localBitrate, frameRate) == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }      
   }
   
   return sr;
}

SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoCpuUsage(const SIPX_INST hInst,
                                                    const int cpuUsage)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoCpuUsage");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoCpuUsage hInst=%p, cpuUsage=%d",
      hInst, cpuUsage);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

      if (pImpl)
      {
         if (pImpl->setVideoCpuValue(cpuUsage) == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}

SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoBitrate(const SIPX_INST hInst,
                                                   const int bitRate)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoBitrate");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoBitrate hInst=%d, bitRate=%d",
      hInst, bitRate);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

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
}


SIPXTAPI_API SIPX_RESULT sipxConfigSetVideoFramerate(const SIPX_INST hInst,
                                                     const int frameRate)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxConfigSetVideoFramerate");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxConfigSetVideoFramerate hInst=%d, frameRate=%d",
      hInst, frameRate);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pImpl = pInst->pCallManager->getMediaInterfaceFactory();

      if (pImpl)
      {
         if (pImpl->setVideoFramerate(frameRate) == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}

/***************************************************************************
* Public Misc Config Functions
***************************************************************************/

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

SIPXTAPI_API SIPX_RESULT sipxConfigGetSessionTimerExpiration(const SIPX_INST hInst,
                                                             int* iSessionInterval) 
{
   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      *iSessionInterval = pInst->pCallManager->getInviteExpireSeconds();
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

SIPXTAPI_API SIPX_RESULT sipxConfigSetSessionTimerExpiration(const SIPX_INST hInst,
                                                             int iSessionInterval) 
{
   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      pInst->pCallManager->setInviteExpireSeconds(iSessionInterval);
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}
