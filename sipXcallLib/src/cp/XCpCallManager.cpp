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
#include <cp/CpMessageTypes.h>
#include <cp/msg/AcCommandMsg.h>
#include <net/SipDialog.h>
#include <net/SipMessageEvent.h>
#include <net/SipLineProvider.h>

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
                               SipUserAgent& rSipUserAgent,
                               SdpCodecFactory& rSdpCodecFactory,
                               SipLineProvider* pSipLineProvider,
                               UtlBoolean bDoNotDisturb,
                               UtlBoolean bEnableICE,
                               UtlBoolean bEnableSipInfo,
                               UtlBoolean bIsRequiredLineMatch,
                               int rtpPortStart,
                               int rtpPortEnd,
                               int maxCalls,
                               int inviteExpireSeconds,
                               CpMediaInterfaceFactory& rMediaInterfaceFactory)
: OsServerTask("XCallManager-%d", NULL, CALLMANAGER_MAX_REQUEST_MSGS)
, m_pCallEventListener(pCallEventListener)
, m_pInfoStatusEventListener(pInfoStatusEventListener)
, m_pSecurityEventListener(pSecurityEventListener)
, m_pMediaEventListener(pMediaEventListener)
, m_rSipUserAgent(rSipUserAgent)
, m_rSdpCodecFactory(rSdpCodecFactory)
, m_pSipLineProvider(pSipLineProvider)
, m_callIdGenerator(callIdPrefix)
, m_conferenceIdGenerator(conferenceIdPrefix)
, m_sipCallIdGenerator(sipCallIdPrefix)
, m_bDoNotDisturb(bDoNotDisturb)
, m_bEnableICE(bEnableICE)
, m_bEnableSipInfo(bEnableSipInfo)
, m_bIsRequiredLineMatch(bIsRequiredLineMatch)
, m_rtpPortStart(rtpPortStart)
, m_rtpPortEnd(rtpPortEnd)
, m_memberMutex(OsMutex::Q_FIFO)
, m_maxCalls(maxCalls)
, m_rMediaInterfaceFactory(rMediaInterfaceFactory)
, m_inviteExpireSeconds(inviteExpireSeconds)
, m_focusMutex(OsMutex::Q_FIFO)
, m_sAbstractCallInFocus(NULL)
{
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_INVITE_METHOD,
      TRUE, // want to get requests
      TRUE, // and responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_BYE_METHOD,
      TRUE, // want to get requests
      TRUE, // and responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_CANCEL_METHOD,
      TRUE, // want to get requests
      TRUE, // and responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_ACK_METHOD,
      TRUE, // want to get requests
      FALSE, // no such thing as a ACK response
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_REFER_METHOD,
      TRUE, // want to get requests
      TRUE, // and responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_OPTIONS_METHOD,
      FALSE, // don't want to get requests
      TRUE, // do want responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_NOTIFY_METHOD,
      TRUE, // do want to get requests
      TRUE, // do want responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_INFO_METHOD,
      TRUE, // do want to get requests
      TRUE, // do want responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages

   // Allow the "replaces" extension, because CallManager
   // implements the INVITE-with-Replaces logic.
   m_rSipUserAgent.allowExtension(SIP_REPLACES_EXTENSION);

   int defaultInviteExpireSeconds = m_rSipUserAgent.getDefaultExpiresSeconds();
   if (m_inviteExpireSeconds > defaultInviteExpireSeconds) m_inviteExpireSeconds = defaultInviteExpireSeconds;
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
   int msgType = rRawMsg.getMsgType();
   int msgSubType = rRawMsg.getMsgSubType();

   switch (msgType)
   {
   case OsMsg::PHONE_APP:
      return handlePhoneAppMessage(rRawMsg);
   case OsMsg::OS_EVENT: // timer event
   default:
      {
         OsSysLog::add(FAC_CP, PRI_ERR, "Unknown TYPE %d of XCpCallManager message subtype: %d\n", msgType, msgSubType);
         bResult = TRUE;
         break;
      }
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

OsStatus XCpCallManager::createCall(UtlString& sCallId)
{
   OsStatus result = OS_FAILED;

   sCallId.remove(0); // clear string

   // always allow creation of new call, check for limit only when establishing
   if (sCallId.isNull())
   {
      sCallId = getNewCallId();
   }

   XCpCall *pCall = new XCpCall(sCallId, m_rSipUserAgent, m_rMediaInterfaceFactory, *getMessageQueue());
   // register listeners
   pCall->setCallEventListener(m_pCallEventListener);
   pCall->setInfoStatusEventListener(m_pInfoStatusEventListener);
   pCall->setSecurityEventListener(m_pSecurityEventListener);
   pCall->setMediaEventListener(m_pMediaEventListener);

   UtlBoolean resStart = pCall->start();
   if (resStart)
   {
      UtlBoolean resPush = push(*pCall);
      if (resPush)
      {
         result = OS_SUCCESS;
      }
      else
      {
         delete pCall; // also shuts down thread
         pCall = NULL;
      }
   }

   return result;
}

OsStatus XCpCallManager::createConference(UtlString& sConferenceId)
{
   OsStatus result = OS_FAILED;

   sConferenceId.remove(0); // clear string

   // always allow creation of new conference, check for limit only when establishing
   if (sConferenceId.isNull())
   {
      sConferenceId = getNewConferenceId();
   }
   XCpConference *pConference = new XCpConference(sConferenceId, m_rSipUserAgent, m_rMediaInterfaceFactory, *getMessageQueue());
   // register listeners
   pConference->setCallEventListener(m_pCallEventListener);
   pConference->setInfoStatusEventListener(m_pInfoStatusEventListener);
   pConference->setSecurityEventListener(m_pSecurityEventListener);
   pConference->setMediaEventListener(m_pMediaEventListener);

   UtlBoolean resStart = pConference->start();
   if (resStart)
   {
      UtlBoolean resPush = push(*pConference);
      if (resPush)
      {
         result = OS_SUCCESS;
      }
      else
      {
         delete pConference; // also shuts down thread
         pConference = NULL;
      }
   }

   return result;
}

OsStatus XCpCallManager::connectCall(const UtlString& sCallId,
                                     SipDialog& sSipDialog,
                                     const UtlString& toAddress,
                                     const UtlString& fullLineUrl,
                                     const UtlString& sSipCallId,
                                     const UtlString& locationHeader,
                                     CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;
   sSipDialog = SipDialog();

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      UtlString sTmpSipCallId = sSipCallId;
      if (sTmpSipCallId.isNull())
      {
         sTmpSipCallId = getNewSipCallId();
      }
      // we found call and have a lock on it
      return ptrLock->connect(sTmpSipCallId, sSipDialog, toAddress, fullLineUrl, locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::connectConferenceCall(const UtlString& sConferenceId,
                                               SipDialog& sSipDialog,
                                               const UtlString& toAddress,
                                               const UtlString& fullLineUrl,
                                               const UtlString& sSipCallId,
                                               const UtlString& locationHeader,
                                               CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;
   sSipDialog = SipDialog();

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      UtlString sTmpSipCallId = sSipCallId;
      if (sTmpSipCallId.isNull())
      {
         sTmpSipCallId = getNewSipCallId();
      }
      // we found call and have a lock on it
      return ptrLock->connect(sTmpSipCallId, sSipDialog, toAddress, fullLineUrl, locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::acceptCallConnection(const UtlString& sCallId,
                                              const UtlString& locationHeader,
                                              CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->acceptConnection(locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::rejectCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->rejectConnection();
   }

   return result;
}

OsStatus XCpCallManager::redirectCallConnection(const UtlString& sCallId,
                                                const UtlString& sRedirectSipUri)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->redirectConnection(sRedirectSipUri);
   }

   return result;
}

OsStatus XCpCallManager::answerCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->answerConnection();
   }

   return result;
}

OsStatus XCpCallManager::dropAbstractCallConnection(const UtlString& sAbstractCallId,
                                                    const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::dropAllAbstractCallConnections(const UtlString& sAbstractCallId)
{
   if (isCallId(sAbstractCallId))
   {
      // call has only 1 connection
      return dropCallConnection(sAbstractCallId);
   }
   else
   {
      // conference has many connections
      return dropAllConferenceConnections(sAbstractCallId);
   }
}

OsStatus XCpCallManager::dropCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection();
   }

   return result;
}

OsStatus XCpCallManager::dropConferenceConnection(const UtlString& sConferenceId, const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::dropAllConferenceConnections(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found conference and have a lock on it
      return ptrLock->dropAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::dropAbstractCall(const UtlString& sAbstractCallId)
{
   if (isCallId(sAbstractCallId))
   {
      return dropCall(sAbstractCallId);
   }
   else
   {
      return dropConference(sAbstractCallId);
   }
}

OsStatus XCpCallManager::dropCall(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection(TRUE);
   }

   return result;
}

OsStatus XCpCallManager::dropConference(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found conference and have a lock on it
      return ptrLock->dropAllConnections(TRUE);
   }

   return result;
}

OsStatus XCpCallManager::destroyAbstractCall(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = deleteAbstractCall(sAbstractCallId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::destroyCall(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = deleteCall(sCallId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::destroyConference(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = deleteConference(sConferenceId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::transferBlindAbstractCall(const UtlString& sAbstractCallId,
                                                   const SipDialog& sSipDialog,
                                                   const UtlString& sTransferSipUrl)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->transferBlind(sSipDialog, sTransferSipUrl);
   }

   return result;
}

OsStatus XCpCallManager::transferConsultativeAbstractCall(const UtlString& sSourceAbstractCallId,
                                                          const SipDialog& sSourceSipDialog,
                                                          const UtlString& sTargetAbstractCallId,
                                                          const SipDialog& sTargetSipDialog)
{
   OsStatus result = OS_FAILED;

   return result;
}

OsStatus XCpCallManager::audioToneStart(const UtlString& sAbstractCallId,
                                        int iToneId,
                                        UtlBoolean bLocal,
                                        UtlBoolean bRemote)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioToneStart(iToneId, bLocal, bRemote);
   }

   return result;
}

OsStatus XCpCallManager::audioToneStop(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioToneStop();
   }

   return result;
}

OsStatus XCpCallManager::audioFilePlay(const UtlString& sAbstractCallId,
                                       const UtlString& audioFile,
                                       UtlBoolean bRepeat,
                                       UtlBoolean bLocal,
                                       UtlBoolean bRemote,
                                       UtlBoolean bMixWithMic /*= FALSE*/,
                                       int iDownScaling /*= 100*/,
                                       void* pCookie /*= NULL*/)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioFilePlay(audioFile, bRepeat, bLocal, bRemote, bMixWithMic, iDownScaling, pCookie);
   }

   return result;
}

OsStatus XCpCallManager::audioBufferPlay(const UtlString& sAbstractCallId,
                                         const void* pAudiobuf,
                                         size_t iBufSize,
                                         int iType,
                                         UtlBoolean bRepeat,
                                         UtlBoolean bLocal,
                                         UtlBoolean bRemote,
                                         void* pCookie /*= NULL*/)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioBufferPlay(pAudiobuf, iBufSize, iType, bRepeat, bLocal, bRemote, pCookie);
   }

   return result;
}

OsStatus XCpCallManager::audioStop(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioStop();
   }

   return result;
}

OsStatus XCpCallManager::pauseAudioPlayback(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->pauseAudioPlayback();
   }

   return result;
}

OsStatus XCpCallManager::resumeAudioPlayback(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->resumeAudioPlayback();
   }

   return result;
}

OsStatus XCpCallManager::audioRecordStart(const UtlString& sAbstractCallId,
                                          const UtlString& sFile)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioRecordStart(sFile);
   }

   return result;
}

OsStatus XCpCallManager::audioRecordStop(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioRecordStop();
   }

   return result;
}

OsStatus XCpCallManager::holdAbstractCallConnection(const UtlString& sAbstractCallId,
                                                    const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::holdCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdConnection();
   }

   return result;
}

OsStatus XCpCallManager::holdAllConferenceConnections(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::holdLocalAbstractCallConnection(const UtlString& sAbstractCallId)
{
   return doYieldFocus(sAbstractCallId, TRUE);
}

OsStatus XCpCallManager::unholdLocalAbstractCallConnection(const UtlString& sAbstractCallId)
{
   return doGainFocus(sAbstractCallId, FALSE);
}

OsStatus XCpCallManager::unholdAllConferenceConnections(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::unholdAbstractCallConnection(const UtlString& sAbstractCallId,
                                                      const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::unholdCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdConnection();
   }

   return result;
}

OsStatus XCpCallManager::muteInputAbstractCallConnection(const UtlString& sAbstractCallId,
                                                         const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->muteInputConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::unmuteInputAbstractCallConnection(const UtlString& sAbstractCallId,
                                                           const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unmuteInputConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::limitAbstractCallCodecPreferences(const UtlString& sAbstractCallId,
                                                           CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                           const UtlString& sAudioCodecs,
                                                           CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                           const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->limitCodecPreferences(audioBandwidthId, sAudioCodecs, videoBandwidthId, sVideoCodecs);
   }

   return result;
}

OsStatus XCpCallManager::renegotiateCodecsAbstractCallConnection(const UtlString& sAbstractCallId,
                                                                 const SipDialog& sSipDialog,
                                                                 CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                                 const UtlString& sAudioCodecs,
                                                                 CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                                 const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->renegotiateCodecsConnection(sSipDialog,
         audioBandwidthId, sAudioCodecs, videoBandwidthId, sVideoCodecs);
   }

   return result;
}

OsStatus XCpCallManager::renegotiateCodecsAllConferenceConnections(const UtlString& sConferenceId,
                                                                   CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                                   const UtlString& sAudioCodecs,
                                                                   CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                                   const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sConferenceId, ptrLock);
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
   OsLock lock(m_memberMutex); // use wide lock to make sure we enable stun for the correct server

   m_sStunServer = sStunServer;
   m_iStunPort = iServerPort;
   m_iStunKeepAlivePeriodSecs = iKeepAlivePeriodSecs;

   m_rSipUserAgent.enableStun(sStunServer, iServerPort, iKeepAlivePeriodSecs, pNotification);
}

void XCpCallManager::enableTurn(const UtlString& sTurnServer,
                                int iTurnPort,
                                const UtlString& sTurnUsername,
                                const UtlString& sTurnPassword,
                                int iKeepAlivePeriodSecs /*= 0*/)
{
   OsLock lock(m_memberMutex);

   bool bEnabled = false;
   m_sTurnServer = sTurnServer;
   m_iTurnPort = iTurnPort;
   m_sTurnUsername = sTurnUsername;
   m_sTurnPassword = sTurnPassword;
   m_iTurnKeepAlivePeriodSecs = iKeepAlivePeriodSecs;
   bEnabled = (m_sTurnServer.length() > 0) && portIsValid(m_iTurnPort);

   m_rSipUserAgent.getContactDb().enableTurn(bEnabled);
}

OsStatus XCpCallManager::sendInfo(const UtlString& sAbstractCallId,
                                  const SipDialog& sSipDialog,
                                  const UtlString& sContentType,
                                  const char* pContent,
                                  const size_t nContentLength)
{
   OsStatus result = OS_FAILED;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->sendInfo(sSipDialog, sContentType, pContent, nContentLength);
   }

   return result;
}

UtlString XCpCallManager::getNewSipCallId()
{
   return m_sipCallIdGenerator.getNewCallId();
}

UtlString XCpCallManager::getNewCallId()
{
   return m_callIdGenerator.getNewCallId();
}

UtlString XCpCallManager::getNewConferenceId()
{
   return m_conferenceIdGenerator.getNewCallId();
}

/* ============================ ACCESSORS ================================= */

CpMediaInterfaceFactory* XCpCallManager::getMediaInterfaceFactory() const
{
   return &m_rMediaInterfaceFactory;
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

OsStatus XCpCallManager::getAbstractCallIds(UtlSList& idList) const
{
   // first append call ids
   getCallIds(idList);
   // then append conference ids
   getConferenceIds(idList);

   return OS_SUCCESS;
}

OsStatus XCpCallManager::getCallIds(UtlSList& callIdList) const
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         callIdList.insert(pCall->getId().clone());
      }
   }

   return OS_SUCCESS;
}

OsStatus XCpCallManager::getConferenceIds(UtlSList& conferenceIdList) const
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         conferenceIdList.insert(pConference->getId().clone());
      }
   }

   return OS_SUCCESS;
}

OsStatus XCpCallManager::getCallSipCallId(const UtlString& sCallId,
                                          UtlString& sSipCallId) const
{
   OsStatus result = OS_NOT_FOUND;
   sSipCallId.remove(0);

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getCallSipCallId(sSipCallId);
   }

   return result;
}

OsStatus XCpCallManager::getConferenceSipCallIds(const UtlString& sConferenceId,
                                                 UtlSList& sipCallIdList) const
{
   OsStatus result = OS_NOT_FOUND;
   sipCallIdList.destroyAll();

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getConferenceSipCallIds(sipCallIdList);
   }

   return result;
}

OsStatus XCpCallManager::getRemoteUserAgent(const UtlString& sAbstractCallId,
                                            const SipDialog& sSipDialog,
                                            UtlString& userAgent) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getRemoteUserAgent(sSipDialog, userAgent);
   }

   return result;
}

OsStatus XCpCallManager::getMediaConnectionId(const UtlString& sAbstractCallId,
                                              int& mediaConnID) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getMediaConnectionId(mediaConnID);
   }

   return result;
}

OsStatus XCpCallManager::getSipDialog(const UtlString& sAbstractCallId,
                                      const SipDialog& sSipDialog,
                                      SipDialog& sOutputSipDialog) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getSipDialog(sSipDialog, sOutputSipDialog);
   }

   return result;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

UtlBoolean XCpCallManager::isCallId(const UtlString& sId)
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   return type == XCpCallManager::ID_TYPE_CALL;
}

UtlBoolean XCpCallManager::isConferenceId(const UtlString& sId)
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   return type == XCpCallManager::ID_TYPE_CONFERENCE;
}

XCpCallManager::ID_TYPE XCpCallManager::getIdType(const UtlString& sId)
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

UtlBoolean XCpCallManager::findAbstractCall(const UtlString& sAbstractCallId,
                                            OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   XCpCallManager::ID_TYPE type = getIdType(sAbstractCallId);
   UtlBoolean result = FALSE;
   switch(type)
   {
   case XCpCallManager::ID_TYPE_CALL:
      {
         OsLock lock(m_memberMutex);
         // cannot reuse existing method, as OsPtrLock<XCpAbstractCall>& cannot be cast to OsPtrLock<XCpCall>&
         XCpCall* pCall = dynamic_cast<XCpCall*>(m_callMap.findValue(&sAbstractCallId));
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
         XCpConference* pConference = dynamic_cast<XCpConference*>(m_conferenceMap.findValue(&sAbstractCallId));
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

UtlBoolean XCpCallManager::findSomeAbstractCall(const UtlString& sAvoidAbstractCallId,
                                                OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   OsLock lock(m_memberMutex);

   // try to get next call
   {
      UtlHashMapIterator callMapItor(m_callMap);
      XCpCall* pCall = NULL;

      while(callMapItor()) // go to next pair
      {
         pCall = dynamic_cast<XCpCall*>(callMapItor.value());
         if (pCall && sAvoidAbstractCallId.compareTo(pCall->getId()) != 0)
         {
            // we found some call and sAvoidAbstractCallId is different than its Id
            ptrLock = pCall; // lock call
            return TRUE;
         }
      }
   }

   // try to get next conference
   {
      UtlHashMapIterator conferenceMapItor(m_conferenceMap);
      XCpConference* pConference = NULL;
      while(conferenceMapItor()) // go to next pair
      {
         pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
         if (pConference && sAvoidAbstractCallId.compareTo(pConference->getId()) != 0)
         {
            // we found some conference and sAvoidAbstractCallId is different than its Id
            ptrLock = pConference; // lock conference
            return TRUE;
         }
      }
   }

   ptrLock = NULL;
   return FALSE;
}

UtlBoolean XCpCallManager::findCall(const SipDialog& sSipDialog,
                                    OsPtrLock<XCpCall>& ptrLock) const
{
   OsLock lock(m_memberMutex);
   XCpCall* pNotEstablishedMatch = NULL;

   // iterate through hashmap and ask every call if it has given sip dialog
   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   // TODO: optimize speed by using list in hashmap indexed by call-id. Have to investigate if call-id switch in XCpCall is possible/desirable
   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         XCpAbstractCall::DialogMatchEnum matchResult = pCall->hasSipDialog(sSipDialog);
         if (matchResult == XCpAbstractCall::ESTABLISHED_MATCH)
         {
            // perfect match, call-id and both tags match
            ptrLock = pCall;
            return TRUE;
         }
         else if (matchResult == XCpAbstractCall::NOT_ESTABLISHED_MATCH)
         {
            // partial match, call-id match but only 1 tag matches, 2nd tag is not present
            pNotEstablishedMatch = pCall; // lock it later
         }
      }
   }

   if (pNotEstablishedMatch)
   {
      ptrLock = pNotEstablishedMatch;
      return TRUE;
   }

   return FALSE;
}

UtlBoolean XCpCallManager::findConference(const SipDialog& sSipDialog,
                                          OsPtrLock<XCpConference>& ptrLock) const
{
   OsLock lock(m_memberMutex);
   XCpConference* pNotEstablishedMatch = NULL;

   // iterate through hashmap and ask every conference if it has given sip dialog
   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         XCpAbstractCall::DialogMatchEnum matchResult = pConference->hasSipDialog(sSipDialog);
         if (matchResult == XCpAbstractCall::ESTABLISHED_MATCH)
         {
            // perfect match, call-id and both tags match
            ptrLock = pConference;
            return TRUE;
         }
         else if (matchResult == XCpAbstractCall::NOT_ESTABLISHED_MATCH)
         {
            // partial match, call-id match but only 1 tag matches, 2nd tag is not present
            pNotEstablishedMatch = pConference; // lock it later
         }
      }
   }

   if (pNotEstablishedMatch)
   {
      ptrLock = pNotEstablishedMatch;
      return TRUE;
   }

   return FALSE;
}

UtlBoolean XCpCallManager::findAbstractCall(const SipDialog& sSipDialog,
                                            OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   OsPtrLock<XCpCall> ptrCallLock; // auto pointer lock
   OsPtrLock<XCpConference> ptrConferenceLock; // auto pointer lock
   UtlBoolean resFind = FALSE;

   // first try to find in calls
   resFind = findCall(sSipDialog, ptrCallLock);
   if (resFind)
   {
      ptrLock = ptrConferenceLock; // move lock
      return TRUE;
   }
   // not found, try conferences
   resFind = findConference(sSipDialog, ptrConferenceLock);
   if (resFind)
   {
      ptrLock = ptrConferenceLock; // move lock
      return TRUE;
   }

   return FALSE;
}

UtlBoolean XCpCallManager::findCall(const UtlString& sId,
                                    OsPtrLock<XCpCall>& ptrLock) const
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

UtlBoolean XCpCallManager::findConference(const UtlString& sId,
                                          OsPtrLock<XCpConference>& ptrLock) const
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

UtlBoolean XCpCallManager::findHandlingAbstractCall(const SipMessage& rSipMessage, OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   SipDialog sipDialog(&rSipMessage);
   return findAbstractCall(sipDialog, ptrLock);
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
   // yield focus
   // nobody will be able to give us focus back after we yield it, because we hold m_memberMutex
   doYieldFocus(sId);

   // avoid findCall, as we don't need that much locking
   UtlContainable *pValue = NULL;
   UtlContainable *pKey = m_callMap.removeKeyAndValue(&sId, pValue);
   if(pKey)
   {
      XCpCall *pCall = dynamic_cast<XCpCall*>(pValue);
      if (pCall)
      {
         // call was found
         pCall->acquireExclusive(); // lock the call exclusively for delete
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
   // yield focus
   // nobody will be able to give us focus back after we yield it, because we hold m_memberMutex
   doYieldFocus(sId);

   // avoid findConference, as we don't need that much locking
   UtlContainable *pValue = NULL;
   UtlContainable *pKey = m_conferenceMap.removeKeyAndValue(&sId, pValue);
   if(pKey)
   {
      XCpConference *pConference = dynamic_cast<XCpConference*>(pValue);
      if (pConference)
      {
         // conference was found
         pConference->acquireExclusive(); // lock the conference exclusively for delete
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

UtlBoolean XCpCallManager::deleteAbstractCall(const UtlString& sAbstractCallId)
{
   XCpCallManager::ID_TYPE type = getIdType(sAbstractCallId);
   UtlBoolean result = FALSE;

   switch(type)
   {
   case XCpCallManager::ID_TYPE_CALL:
      {
         deleteCall(sAbstractCallId);
      }
   case XCpCallManager::ID_TYPE_CONFERENCE:
      {
         deleteConference(sAbstractCallId);
      }
   default:
      break;
   }

   return result;
}

void XCpCallManager::deleteAllCalls()
{
   doYieldFocus(FALSE);
   OsLock lock(m_memberMutex);
   m_callMap.destroyAll();
}

void XCpCallManager::deleteAllConferences()
{
   doYieldFocus(FALSE);
   OsLock lock(m_memberMutex);
   m_conferenceMap.destroyAll();
}

UtlBoolean XCpCallManager::checkCallLimit()
{
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

// handles OsMsg::PHONE_APP messages
UtlBoolean XCpCallManager::handlePhoneAppMessage(const OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;
   int msgSubType = rRawMsg.getMsgSubType();

   switch (msgSubType)
   {
   case SipMessage::NET_SIP_MESSAGE:
      {
         const SipMessageEvent* pSipMsgEvent = dynamic_cast<const SipMessageEvent*>(&rRawMsg);
         if (pSipMsgEvent)
         {
            return handleSipMessageEvent(*pSipMsgEvent);
         }
         else
         {
            OsSysLog::add(FAC_CP, PRI_ERR, "Invalid SipMessage::NET_SIP_MESSAGE, cannot be cast to SipMessageEvent\n");
            bResult = TRUE;
            break;
         }
      }
   default:
      {
         OsSysLog::add(FAC_CP, PRI_ERR, "Unknown PHONE_APP CallManager message subtype: %d\n", msgSubType);
         bResult = TRUE;
         break;
      }
   }

   return bResult;
}

UtlBoolean XCpCallManager::handleSipMessageEvent(const SipMessageEvent& rSipMsgEvent)
{
   const SipMessage* pSipMessage = rSipMsgEvent.getMessage();
   if (pSipMessage)
   {
#ifdef PRINT_SIP_MESSAGE
      osPrintf("\nXCpCallManager::handleSipMessageEvent\n%s\n-----------------------------------\n", pSipMessage->toString().data());
#endif
      OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
      UtlBoolean resFind = findHandlingAbstractCall(*pSipMessage, ptrLock);
      if (resFind)
      {
         // post message to call
         return ptrLock->postMessage(rSipMsgEvent);
      }
      else
      {
         // SipMessage must meet certain basic conditions to be processed - be a request (but not ACK)
         if (basicSipMessageEventCheck(rSipMsgEvent))
         {
            // no call found, handle SipMessage
            return handleUnknownSipMessageEvent(rSipMsgEvent);
         }
      }
   }

   return TRUE;
}

// handles SipMessages that are requests (except ACK)
UtlBoolean XCpCallManager::handleUnknownSipMessageEvent(const SipMessageEvent& rSipMsgEvent)
{
   const SipMessage* pSipMessage = rSipMsgEvent.getMessage();

   if (pSipMessage)
   {
      // maybe check if line exists
      if(m_pSipLineProvider && m_bIsRequiredLineMatch)
      {
         UtlBoolean isLineValid = m_pSipLineProvider->lineExists(*pSipMessage);
         if (!isLineValid)
         {
            // no such user - return 404
            SipMessage noSuchUserResponse;
            noSuchUserResponse.setResponseData(pSipMessage,
               SIP_NOT_FOUND_CODE,
               SIP_NOT_FOUND_TEXT);
            m_rSipUserAgent.send(noSuchUserResponse);
            return TRUE;
         }
      }
      // check if we didn't reach call limit
      if (checkCallLimit())
      {
         if (!m_bDoNotDisturb)
         {
            // all checks passed, create new call
            createNewCall(rSipMsgEvent);
         }
         else
         {
            UtlString requestUri;
            pSipMessage->getRequestUri(&requestUri);
            OsSysLog::add(FAC_CP, PRI_DEBUG, "XCpCallManager::handleSipMessage - Rejecting inbound call to %s due to DND mode", requestUri.data());
            // send 486 Busy here
            SipMessage busyHereResponse;
            busyHereResponse.setInviteBusyData(pSipMessage);
            m_rSipUserAgent.send(busyHereResponse);
         }
      }
      else
      {
         OsSysLog::add(FAC_CP, PRI_WARNING, "XCpCallManager::handleSipMessage - The call stack size as reached it's limit of %d", m_maxCalls);
         // send 486 Busy here
         SipMessage busyHereResponse;
         busyHereResponse.setInviteBusyData(pSipMessage);
         m_rSipUserAgent.send(busyHereResponse);
      }
   }

   return TRUE;
}

UtlBoolean XCpCallManager::basicSipMessageEventCheck(const SipMessageEvent& rSipMsgEvent)
{
   int messageType = rSipMsgEvent.getMessageStatus();

   switch(messageType)
   {
      // This is a request which failed to get sent
   case SipMessageEvent::TRANSPORT_ERROR:
   case SipMessageEvent::SESSION_REINVITE_TIMER:
   case SipMessageEvent::AUTHENTICATION_RETRY:
      // Ignore it and do not create a call
      return FALSE;
   default:
      {
         const SipMessage* pSipMessage = rSipMsgEvent.getMessage();
         if (pSipMessage)
         {
            // Its a SIP Request
            if(pSipMessage->isRequest())
            {
               return basicSipMessageRequestCheck(*pSipMessage);
            }
         }
      }
   }

   return FALSE;
}

UtlBoolean XCpCallManager::basicSipMessageRequestCheck(const SipMessage& rSipMessage)
{
   UtlString requestMethod;
   rSipMessage.getRequestMethod(&requestMethod);
   Url toUrl;
   rSipMessage.getToUrl(toUrl);
   UtlString toTag;
   toUrl.getFieldParameter("tag", toTag);

   // Dangling or delayed ACK
   if(requestMethod.compareTo(SIP_ACK_METHOD) == 0)
   {
      return FALSE;
   }
   else if(requestMethod.compareTo(SIP_INVITE_METHOD) == 0 && toTag.isNull())
   {
      // handle INVITE without to tag
      return TRUE;
   }
   else if(requestMethod.compareTo(SIP_REFER_METHOD) == 0)
   {
      // handle refer
      return TRUE;
   }

   // Send a bad callId/transaction message
   SipMessage badTransactionMessage;
   badTransactionMessage.setBadTransactionData(&rSipMessage);
   m_rSipUserAgent.send(badTransactionMessage);
   return FALSE;
}

void XCpCallManager::createNewCall(const SipMessageEvent& rSipMsgEvent)
{
   const SipMessage* pSipMessage = rSipMsgEvent.getMessage();
   if (pSipMessage)
   {
      UtlString sSipCallId = getNewSipCallId();

      XCpCall* pCall = new XCpCall(sSipCallId, m_rSipUserAgent, m_rMediaInterfaceFactory, *getMessageQueue());
      // register listeners
      pCall->setCallEventListener(m_pCallEventListener);
      pCall->setInfoStatusEventListener(m_pInfoStatusEventListener);
      pCall->setSecurityEventListener(m_pSecurityEventListener);
      pCall->setMediaEventListener(m_pMediaEventListener);

      UtlBoolean resStart = pCall->start(); // start thread
      if (resStart)
      {
         pCall->postMessage(rSipMsgEvent); // repost message into thread
         UtlBoolean resPush = push(*pCall); // inserts call into list of calls
         if (resPush)
         {
            if (OsSysLog::willLog(FAC_CP, PRI_DEBUG))
            {
               UtlString requestUri;
               pSipMessage->getRequestUri(&requestUri);
               UtlString fromField;
               pSipMessage->getFromField(&fromField);
               OsSysLog::add(FAC_CP, PRI_DEBUG, "XCpCallManager::createNewCall - Creating new call destined to %s from %s", requestUri.data(), fromField.data());
            }
         }
         else
         {
            OsSysLog::add(FAC_CP, PRI_ERR, "XCpCallManager::createNewCall - Couldn't push call on stack");
            pCall->requestShutdown();
            delete pCall;
         }
      }
      else
      {
         OsSysLog::add(FAC_CP, PRI_ERR, "XCpCallManager::createNewCall - Call thread could not be started");
      }
   }
}

OsStatus XCpCallManager::doGainFocus(const UtlString& sAbstractCallId, UtlBoolean bGainOnlyIfNoFocusedCall)
{
#ifndef DISABLE_LOCAL_AUDIO
   OsStatus result = OS_FAILED;
   OsLock lock(m_focusMutex);

   if (!m_sAbstractCallInFocus.isNull())
   {
      // some call is focused
      if (bGainOnlyIfNoFocusedCall)
      {
         // some call is focused, then we don't want to focus
         return OS_SUCCESS;
      }
      result = doYieldFocus(sAbstractCallId, FALSE); // defocus focused call
   }

   {
      OsPtrLock<XCpAbstractCall> ptrLock; // scoped auto pointer lock
      UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
      if (resFind)
      {
         // send gain focus command to new call
         AcCommandMsg gainFocusCommand(AcCommandMsg::AC_GAIN_FOCUS);
         // we found call and have a lock on it
         ptrLock->postMessage(gainFocusCommand);
         m_sAbstractCallInFocus = sAbstractCallId;
         result = OS_SUCCESS;
      }
   }

   return result;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpCallManager::doYieldFocus(const UtlString& sAbstractCallId,
                                      UtlBoolean bShiftFocus)
{
#ifndef DISABLE_LOCAL_AUDIO
   OsStatus result = OS_FAILED;
   OsLock lock(m_focusMutex); // need to hold lock of the whole time to ensure consistency

   if (m_sAbstractCallInFocus.compareTo(sAbstractCallId) == 0)
   {
      {
         OsPtrLock<XCpAbstractCall> ptrLock; // scoped auto pointer lock
         UtlBoolean resFind = findAbstractCall(m_sAbstractCallInFocus, ptrLock);
         if (resFind)
         {
            // send defocus command to old call
            AcCommandMsg defocusCommand(AcCommandMsg::AC_YIELD_FOCUS);
            // we found call and have a lock on it
            ptrLock->postMessage(defocusCommand);
            result = OS_SUCCESS;
         }
      }

      if (bShiftFocus)
      {
         UtlString sAvoidAbstractCallId(m_sAbstractCallInFocus);
         m_sAbstractCallInFocus.remove(0);
         result = doGainNextFocus(sAvoidAbstractCallId);
      }
      else
      {
         m_sAbstractCallInFocus.remove(0);
      }
   }

   return result;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpCallManager::doYieldFocus(UtlBoolean bShiftFocus)
{
#ifndef DISABLE_LOCAL_AUDIO
   OsStatus result = OS_FAILED;
   OsLock lock(m_focusMutex); // need to hold lock of the whole time to ensure consistency

   {
      OsPtrLock<XCpAbstractCall> ptrLock; // scoped auto pointer lock
      UtlBoolean resFind = findAbstractCall(m_sAbstractCallInFocus, ptrLock);
      if (resFind)
      {
         // send defocus command to old call
         AcCommandMsg defocusCommand(AcCommandMsg::AC_YIELD_FOCUS);
         // we found call and have a lock on it
         ptrLock->postMessage(defocusCommand);
         result = OS_SUCCESS;
      }
   }

   if (bShiftFocus)
   {
      UtlString sAvoidAbstractCallId(m_sAbstractCallInFocus);
      m_sAbstractCallInFocus.remove(0);
      result = doGainNextFocus(sAvoidAbstractCallId);
   }
   else
   {
      m_sAbstractCallInFocus.remove(0);
   }

   return result;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpCallManager::doGainNextFocus(const UtlString& sAvoidAbstractCallId)
{
#ifndef DISABLE_LOCAL_AUDIO
   OsStatus result = OS_FAILED;
   OsLock lock(m_focusMutex); // need to hold lock of the whole time to ensure consistency

   if (m_sAbstractCallInFocus.isNull())
   {
      // no call has focus
      OsPtrLock<XCpAbstractCall> ptrLock; // scoped auto pointer lock
      UtlBoolean resFind = findSomeAbstractCall(sAvoidAbstractCallId, ptrLock);// avoids sAvoidAbstractCallId when looking for next call
      if (resFind)
      {
         // send gain focus command to new call
         AcCommandMsg gainFocusCommand(AcCommandMsg::AC_GAIN_FOCUS);
         // we found call and have a lock on it
         ptrLock->postMessage(gainFocusCommand);
         m_sAbstractCallInFocus = ptrLock->getId();
         result = OS_SUCCESS;
      }
   }

   return result;
#else
   return OS_SUCCESS;
#endif
}

/* ============================ FUNCTIONS ================================= */
