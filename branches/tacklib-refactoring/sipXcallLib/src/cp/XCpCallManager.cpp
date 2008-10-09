//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include <os/OsLock.h>
#include <os/OsPtrLock.h>
#include <utl/UtlHashMapIterator.h>
#include <utl/UtlSList.h>
#include <utl/UtlInt.h>
#include <net/SipUserAgent.h>
#include <cp/XCpCallManager.h>
#include <cp/XCpAbstractCall.h>
#include <cp/XCpCall.h>
#include <cp/XCpConference.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
const char* callIdPrefix = "call_"; // id prefix for all calls. Only used internally.
const char* conferenceIdPrefix = "conf_"; // id prefix for all conferences. Only used internally.
const char* sipCallIdPrefix = "s"; // prefix of sip call-id sent in sip messages.

// STATIC VARIABLE INITIALIZATIONS
const int XCpCallManager::CALLMANAGER_MAX_REQUEST_MSGS = 2000;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XCpCallManager::XCpCallManager(CpCallStateEventListener* pCallEventListener,
                               SipInfoStatusEventListener* pInfoStatusEventListener,
                               SipSecurityEventListener* pSecurityEventListener,
                               CpMediaEventListener* pMediaEventListener,
                               SipUserAgent* pSipUserAgent,
                               UtlBoolean bDoNotDisturb,
                               UtlBoolean bEnableICE,
                               UtlBoolean bEnableSipInfo,
                               int rtpPortStart,
                               int rtpPortEnd,
                               int maxCalls,
                               CpMediaInterfaceFactory* pMediaFactory)
: OsServerTask("XCallManager-%d", NULL, CALLMANAGER_MAX_REQUEST_MSGS)
, m_pCallEventListener(pCallEventListener)
, m_pInfoStatusEventListener(pInfoStatusEventListener)
, m_pSecurityEventListener(pSecurityEventListener)
, m_pMediaEventListener(pMediaEventListener)
, m_pSipUserAgent(pSipUserAgent)
, m_callIdGenerator(callIdPrefix)
, m_conferenceIdGenerator(conferenceIdPrefix)
, m_sipCallIdGenerator(sipCallIdPrefix)
, m_bDoNotDisturb(bDoNotDisturb)
, m_bEnableICE(bEnableICE)
, m_bEnableSipInfo(bEnableSipInfo)
, m_rtpPortStart(rtpPortStart)
, m_rtpPortEnd(rtpPortEnd)
, m_memberMutex(OsMutex::Q_FIFO)
, m_maxCalls(maxCalls)
, m_pMediaFactory(pMediaFactory)
{

}

XCpCallManager::~XCpCallManager()
{
   waitUntilShutDown();
   deleteAllCalls();
   deleteAllConferences();
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean XCpCallManager::handleMessage(OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;

   switch (rRawMsg.getMsgType())
   {
   case 1:
   default:
      break;
   }

   return bResult;
}

void XCpCallManager::requestShutdown(void)
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         pCall->requestShutdown();
      }
   }

   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         pConference->requestShutdown();
      }
   }

   OsServerTask::requestShutdown();
}

OsStatus XCpCallManager::createCall(UtlString& id)
{
   OsStatus result = OS_FAILED;

   id.remove(0); // clear string

   // always allow creation of new call, check for limit only when establishing
   UtlString callId = getNewCallId();
   XCpCall *pCall = new XCpCall(callId);
   UtlBoolean resStart = pCall->start();
   if (resStart)
   {
      UtlBoolean resPush = push(*pCall);
      if (resPush)
      {
         result = OS_SUCCESS;
         id = callId;
      }
      else
      {
         delete pCall; // also shuts down thread
         pCall = NULL;
      }
   }

   return result;
}

OsStatus XCpCallManager::createConference(UtlString& id)
{
   OsStatus result = OS_FAILED;

   id.remove(0); // clear string

   // always allow creation of new conference, check for limit only when establishing
   UtlString conferenceId = getNewConferenceId();
   XCpConference *pConference = new XCpConference(conferenceId);
   UtlBoolean resStart = pConference->start();
   if (resStart)
   {
      UtlBoolean resPush = push(*pConference);
      if (resPush)
      {
         result = OS_SUCCESS;
         id = conferenceId;
      }
      else
      {
         delete pConference; // also shuts down thread
         pConference = NULL;
      }
   }

   return result;
}

OsStatus XCpCallManager::connectCall(const UtlString& sId,
                                     UtlString& sSipCallId,
                                     const UtlString& toAddress,
                                     const UtlString& lineURI,
                                     const UtlString& locationHeader,
                                     CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;
   sSipCallId.remove(0); // clear string

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      sSipCallId = getNewSipCallId();
      // we found call and have a lock on it
      return ptrLock->connect(sSipCallId, toAddress, lineURI, locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::connectConferenceCall(const UtlString& sId,
                                               UtlString& sSipCallId,
                                               const UtlString& toAddress,
                                               const UtlString& lineURI,
                                               const UtlString& locationHeader,
                                               CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;
   sSipCallId.remove(0); // clear string

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sId, ptrLock);
   if (resFind)
   {
      sSipCallId = getNewSipCallId();
      // we found call and have a lock on it
      return ptrLock->connect(sSipCallId, toAddress, lineURI, locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::acceptCallConnection(const UtlString& sId,
                                              const UtlString& locationHeader,
                                              CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->acceptConnection(locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::rejectCallConnection(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->rejectConnection();
   }

   return result;
}

OsStatus XCpCallManager::redirectCallConnection(const UtlString& sId,
                                                const UtlString& sRedirectSipUri)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->redirectConnection(sRedirectSipUri);
   }

   return result;
}

OsStatus XCpCallManager::answerCallConnection(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->answerConnection();
   }

   return result;
}

OsStatus XCpCallManager::dropAbstractCallConnection(const UtlString& sId,
                                                    const UtlString& sSipCallId,
                                                    const UtlString& sLocalTag,
                                                    const UtlString& sRemoteTag)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection(sSipCallId, sLocalTag, sRemoteTag);
   }

   return result;
}

OsStatus XCpCallManager::dropCallConnection(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection();
   }

   return result;
}

OsStatus XCpCallManager::dropAllConferenceConnections(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sId, ptrLock);
   if (resFind)
   {
      // we found conference and have a lock on it
      return ptrLock->dropAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::dropAbstractCall(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = deleteAbstractCall(sId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::dropCall(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = deleteCall(sId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::dropConference(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = deleteConference(sId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::holdAbstractCallConnection(const UtlString& sId,
                                                    const UtlString& sSipCallId,
                                                    const UtlString& sLocalTag,
                                                    const UtlString& sRemoteTag)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdConnection(sSipCallId, sLocalTag, sRemoteTag);
   }

   return result;
}

OsStatus XCpCallManager::holdCallConnection(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdConnection();
   }

   return result;
}

OsStatus XCpCallManager::holdAllConferenceConnections(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::holdLocalAbstractCallConnection(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdLocalConnection();
   }

   return result;
}

OsStatus XCpCallManager::unholdLocalAbstractCallConnection(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdLocalConnection();
   }

   return result;
}

OsStatus XCpCallManager::unholdAllConferenceConnections(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::unholdAbstractCallConnection(const UtlString& sId,
                                                      const UtlString& sSipCallId,
                                                      const UtlString& sLocalTag,
                                                      const UtlString& sRemoteTag)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdConnection(sSipCallId, sLocalTag, sRemoteTag);
   }

   return result;
}

OsStatus XCpCallManager::unholdCallConnection(const UtlString& sId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdConnection();
   }

   return result;
}

OsStatus XCpCallManager::silentHoldRemoteAbstractCallConnection(const UtlString& sId,
                                                                const UtlString& sSipCallId,
                                                                const UtlString& sLocalTag,
                                                                const UtlString& sRemoteTag)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->silentHoldRemoteConnection(sSipCallId, sLocalTag, sRemoteTag);
   }

   return result;
}

OsStatus XCpCallManager::silentUnholdRemoteAbstractCallConnection(const UtlString& sId,
                                                                  const UtlString& sSipCallId,
                                                                  const UtlString& sLocalTag,
                                                                  const UtlString& sRemoteTag)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->silentUnholdRemoteConnection(sSipCallId, sLocalTag, sRemoteTag);
   }

   return result;
}

OsStatus XCpCallManager::silentHoldLocalAbstractCallConnection(const UtlString& sId,
                                                               const UtlString& sSipCallId,
                                                               const UtlString& sLocalTag,
                                                               const UtlString& sRemoteTag)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->silentHoldLocalConnection(sSipCallId, sLocalTag, sRemoteTag);
   }

   return result;
}

OsStatus XCpCallManager::silentUnholdLocalAbstractCallConnection(const UtlString& sId,
                                                                 const UtlString& sSipCallId,
                                                                 const UtlString& sLocalTag,
                                                                 const UtlString& sRemoteTag)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->silentUnholdLocalConnection(sSipCallId, sLocalTag, sRemoteTag);
   }

   return result;
}

OsStatus XCpCallManager::limitAbstractCallCodecPreferences(const UtlString& sId,
                                                           CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                           const UtlString& sAudioCodecs,
                                                           CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                           const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->limitCodecPreferences(audioBandwidthId, sAudioCodecs, videoBandwidthId, sVideoCodecs);
   }

   return result;
}

OsStatus XCpCallManager::renegotiateCodecsAbstractCallConnection(const UtlString& sId,
                                                                 const UtlString& sSipCallId,
                                                                 const UtlString& sLocalTag, 
                                                                 const UtlString& sRemoteTag,
                                                                 CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                                 const UtlString& sAudioCodecs,
                                                                 CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                                 const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->renegotiateCodecsConnection(sSipCallId, sLocalTag, sRemoteTag,
         audioBandwidthId, sAudioCodecs, videoBandwidthId, sVideoCodecs);
   }

   return result;
}

OsStatus XCpCallManager::renegotiateCodecsAllConferenceConnections(const UtlString& sId,
                                                                   CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                                   const UtlString& sAudioCodecs,
                                                                   CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                                   const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->renegotiateCodecsAllConnections(audioBandwidthId, sAudioCodecs, videoBandwidthId, sVideoCodecs);
   }

   return result;
}

void XCpCallManager::enableStun(const UtlString& sStunServer,
                                int iServerPort,
                                int iKeepAlivePeriodSecs /*= 0*/,
                                OsNotification* pNotification /*= NULL*/)
{
   if (m_pSipUserAgent)
   {
      OsLock lock(m_memberMutex); // use wide lock to make sure we enable stun for the correct server

      m_sStunServer = sStunServer;
      m_iStunPort = iServerPort;
      m_iStunKeepAlivePeriodSecs = iKeepAlivePeriodSecs;

      m_pSipUserAgent->enableStun(sStunServer, iServerPort, iKeepAlivePeriodSecs, pNotification);
   }
}

void XCpCallManager::enableTurn(const UtlString& sTurnServer,
                                int iTurnPort,
                                const UtlString& sTurnUsername,
                                const UtlString& sTurnPassword,
                                int iKeepAlivePeriodSecs /*= 0*/)
{
   if (m_pSipUserAgent)
   {
      OsLock lock(m_memberMutex);

      bool bEnabled = false;
      m_sTurnServer = sTurnServer;
      m_iTurnPort = iTurnPort;
      m_sTurnUsername = sTurnUsername;
      m_sTurnPassword = sTurnPassword;
      m_iTurnKeepAlivePeriodSecs = iKeepAlivePeriodSecs;
      bEnabled = (m_sTurnServer.length() > 0) && portIsValid(m_iTurnPort);

      m_pSipUserAgent->getContactDb().enableTurn(bEnabled);
   }
}

OsStatus XCpCallManager::sendInfo(const UtlString& sId,
                                  const UtlString& sSipCallId,
                                  const UtlString& sLocalTag,
                                  const UtlString& sRemoteTag,
                                  const UtlString& sContentType,
                                  const UtlString& sContentEncoding,
                                  const UtlString& sContent)
{
   OsStatus result = OS_FAILED;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->sendInfo(sSipCallId, sLocalTag, sRemoteTag, sContentType, sContentEncoding, sContent);
   }

   return result;
}

/* ============================ ACCESSORS ================================= */

CpMediaInterfaceFactory* XCpCallManager::getMediaInterfaceFactory() const
{
   return m_pMediaFactory;
}

/* ============================ INQUIRY =================================== */

int XCpCallManager::getCallCount() const
{
   OsLock lock(m_memberMutex);
   int count = 0;

   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         count += pCall->getCallCount();
      }
   }

   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         count += pConference->getCallCount();
      }
   }

   return count;
}

OsStatus XCpCallManager::getCallIds(UtlSList& idList) const
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         idList.insert(pCall->getId().clone());
      }
   }

   return OS_SUCCESS;
}

OsStatus XCpCallManager::getConferenceIds(UtlSList& idList) const
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         idList.insert(pConference->getId().clone());
      }
   }

   return OS_SUCCESS;
}

OsStatus XCpCallManager::getCallSipCallId(const UtlString& sId,
                                          UtlString& sSipCallId) const
{
   OsStatus result = OS_NOT_FOUND;
   sSipCallId.remove(0);

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getCallSipCallId(sSipCallId);
   }

   return result;
}

OsStatus XCpCallManager::getConferenceSipCallIds(const UtlString& sId,
                                                 UtlSList& sipCallIdList) const
{
   OsStatus result = OS_NOT_FOUND;
   sipCallIdList.destroyAll();

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getConferenceSipCallIds(sipCallIdList);
   }

   return result;
}

OsStatus XCpCallManager::getAudioEnergyLevels(const UtlString& sId,
                                              int& iInputEnergyLevel,
                                              int& iOutputEnergyLevel) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getAudioEnergyLevels(iInputEnergyLevel, iOutputEnergyLevel);
   }

   return result;
}

OsStatus XCpCallManager::getRemoteUserAgent(const UtlString& sId,
                                            const UtlString& sSipCallId,
                                            const UtlString& sLocalTag,
                                            const UtlString& sRemoteTag,
                                            UtlString& userAgent) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getRemoteUserAgent(sSipCallId, sLocalTag, sRemoteTag, userAgent);
   }

   return result;
}

OsStatus XCpCallManager::getMediaConnectionId(const UtlString& sId, int& mediaConnID) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getMediaConnectionId(mediaConnID);
   }

   return result;
}

OsStatus XCpCallManager::getSipDialog(const UtlString& sId,
                                      const UtlString& sSipCallId,
                                      const UtlString& sLocalTag,
                                      const UtlString& sRemoteTag,
                                      SipDialog& dialog) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCallById(sId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getSipDialog(sSipCallId, sLocalTag, sRemoteTag, dialog);
   }

   return result;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

UtlString XCpCallManager::getNewCallId()
{
   return m_callIdGenerator.getNewCallId();
}

UtlString XCpCallManager::getNewConferenceId()
{
   return m_conferenceIdGenerator.getNewCallId();
}

UtlString XCpCallManager::getNewSipCallId()
{
   return m_sipCallIdGenerator.getNewCallId();
}

UtlBoolean XCpCallManager::isCallId(const UtlString& sId) const
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   return type == XCpCallManager::ID_TYPE_CALL;
}

UtlBoolean XCpCallManager::isConferenceId(const UtlString& sId) const
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   return type == XCpCallManager::ID_TYPE_CONFERENCE;
}

XCpCallManager::ID_TYPE XCpCallManager::getIdType(const UtlString& sId) const
{
   if (sId.first(callIdPrefix) >= 0)
   {
      return XCpCallManager::ID_TYPE_CALL;
   }
   else if (sId.first(conferenceIdPrefix) >= 0)
   {
      return XCpCallManager::ID_TYPE_CONFERENCE;
   }
   else return XCpCallManager::ID_TYPE_UNKNOWN;
}

UtlBoolean XCpCallManager::findAbstractCallById(const UtlString& sId, OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   UtlBoolean result = FALSE;
   switch(type)
   {
   case XCpCallManager::ID_TYPE_CALL:
      {
         OsLock lock(m_memberMutex);
         // cannot reuse existing method, as OsPtrLock<XCpAbstractCall>& cannot be cast to OsPtrLock<XCpCall>&
         XCpCall* pCall = dynamic_cast<XCpCall*>(m_callMap.findValue(&sId));
         if (pCall)
         {
            ptrLock = pCall;
            return TRUE;
         }

         ptrLock = NULL;
         return FALSE;
      }
   case XCpCallManager::ID_TYPE_CONFERENCE:
      {
         OsLock lock(m_memberMutex);
         // cannot reuse existing method, as OsPtrLock<XCpAbstractCall>& cannot be cast to OsPtrLock<XCpConference>&
         XCpConference* pConference = dynamic_cast<XCpConference*>(m_conferenceMap.findValue(&sId));
         if (pConference)
         {
            ptrLock = pConference;
            return TRUE;
         }

         ptrLock = NULL;
         return FALSE;
      }
   default:
      break;
   }

   ptrLock = NULL;
   return result;
}

UtlBoolean XCpCallManager::findAbstractCallBySipDialog(const UtlString& sSipCallId,
                                                       const UtlString& sLocalTag,
                                                       const UtlString& sRemoteTag,
                                                       OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   OsLock lock(m_memberMutex);

   // iterate through hashmap and ask every call if it has given sip dialog
   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   // TODO: optimize speed by using list in hashmap indexed by call-id. Have to investigate if call-id switch in XCpCall is possible/desirable
   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall && pCall->hasSipDialog(sSipCallId, sLocalTag, sRemoteTag))
      {
         ptrLock = pCall;
         return TRUE;
      }
   }

   // iterate through hashmap and ask every conference if it has given sip dialog
   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference && pConference->hasSipDialog(sSipCallId, sLocalTag, sRemoteTag))
      {
         ptrLock = pConference;
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean XCpCallManager::findCall(const UtlString& sId, OsPtrLock<XCpCall>& ptrLock) const
{
   OsLock lock(m_memberMutex);
   XCpCall* pCall = dynamic_cast<XCpCall*>(m_callMap.findValue(&sId));
   if (pCall)
   {
      ptrLock = pCall;
      return TRUE;
   }

   ptrLock = NULL;
   return FALSE;
}

UtlBoolean XCpCallManager::findConference(const UtlString& sId, OsPtrLock<XCpConference>& ptrLock) const
{
   OsLock lock(m_memberMutex);
   XCpConference* pConference = dynamic_cast<XCpConference*>(m_conferenceMap.findValue(&sId));
   if (pConference)
   {
      ptrLock = pConference;
      return TRUE;
   }

   ptrLock = NULL;
   return FALSE;
}

UtlBoolean XCpCallManager::push(XCpCall& call)
{
   OsLock lock(m_memberMutex);

   UtlCopyableContainable *pKey = call.getId().clone();
   UtlContainable *pResult = m_callMap.insertKeyAndValue(pKey, &call);
   if (pResult)
   {
      return TRUE;
   }
   else
   {
      delete pKey;
      pKey = NULL;
      return FALSE;
   }
}

UtlBoolean XCpCallManager::push(XCpConference& conference)
{
   OsLock lock(m_memberMutex);

   UtlCopyableContainable *pKey = conference.getId().clone();
   UtlContainable *pResult = m_conferenceMap.insertKeyAndValue(pKey, &conference);
   if (pResult)
   {
      return TRUE;
   }
   else
   {
      delete pKey;
      pKey = NULL;
      return FALSE;
   }
}

UtlBoolean XCpCallManager::deleteCall(const UtlString& sId)
{
   OsLock lock(m_memberMutex);
   // avoid findCall, as we don't need that much locking
   UtlContainable *pValue = NULL;
   UtlContainable *pKey = m_callMap.removeKeyAndValue(&sId, pValue);
   if(pKey)
   {
      XCpCall *pCall = dynamic_cast<XCpCall*>(pValue);
      if (pCall)
      {
         // call was found
         pCall->acquire(); // lock the call
         delete pCall;
         pCall = NULL;
      }
      else
      {
         OsSysLog::add(FAC_CP, PRI_WARNING, "Unexpected state. Key was removed from call hashmap but value was NULL.");
      }
      delete pKey;
      pKey = NULL;
      return TRUE;
   }
   return FALSE;
}

UtlBoolean XCpCallManager::deleteConference(const UtlString& sId)
{
   OsLock lock(m_memberMutex);
   // avoid findConference, as we don't need that much locking
   UtlContainable *pValue = NULL;
   UtlContainable *pKey = m_conferenceMap.removeKeyAndValue(&sId, pValue);
   if(pKey)
   {
      XCpConference *pConference = dynamic_cast<XCpConference*>(pValue);
      if (pConference)
      {
         // conference was found
         pConference->acquire(); // lock the conference
         delete pConference;
         pConference = NULL;
      }
      else
      {
         OsSysLog::add(FAC_CP, PRI_WARNING, "Unexpected state. Key was removed from conference hashmap but value was NULL.");
      }
      delete pKey;
      pKey = NULL;
      return TRUE;
   }
   return FALSE;
}

UtlBoolean XCpCallManager::deleteAbstractCall(const UtlString& sId)
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   UtlBoolean result = FALSE;

   switch(type)
   {
   case XCpCallManager::ID_TYPE_CALL:
      {
         deleteCall(sId);
      }
   case XCpCallManager::ID_TYPE_CONFERENCE:
      {
         deleteConference(sId);
      }
   default:
      break;
   }

   return result;
}

void XCpCallManager::deleteAllCalls()
{
   OsLock lock(m_memberMutex);
   m_callMap.destroyAll();
}

void XCpCallManager::deleteAllConferences()
{
   OsLock lock(m_memberMutex);
   m_conferenceMap.destroyAll();
}

UtlBoolean XCpCallManager::canCreateNewCall()
{
   if (m_bDoNotDisturb)
   {
      return FALSE;
   }
   if (m_maxCalls == -1)
   {
      return TRUE;
   }

   {
      OsLock lock(m_memberMutex);
      int callCount = getCallCount();
      if (callCount >= m_maxCalls)
      {
         return FALSE;
      }
   }

   return TRUE;
}

/* ============================ FUNCTIONS ================================= */
