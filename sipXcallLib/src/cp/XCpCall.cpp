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
#include <os/OsLock.h>
#include <os/OsPtrLock.h>
#include <os/OsIntPtrMsg.h>
#include <net/SipDialog.h>
#include <cp/XCpCall.h>
#include <cp/XSipConnection.h>
#include <cp/msg/AcConnectMsg.h>
#include <cp/msg/AcAcceptConnectionMsg.h>
#include <cp/msg/AcAnswerConnectionMsg.h>
#include <cp/msg/AcRedirectConnectionMsg.h>
#include <cp/msg/AcRejectConnectionMsg.h>
#include <cp/msg/AcDropConnectionMsg.h>
#include <cp/msg/AcHoldConnectionMsg.h>
#include <cp/msg/AcUnholdConnectionMsg.h>
#include <cp/msg/AcTransferBlindMsg.h>
#include <cp/msg/AcRenegotiateCodecsMsg.h>
#include <cp/msg/AcSendInfoMsg.h>
#include <cp/msg/CpTimerMsg.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XCpCall::XCpCall(const UtlString& sId,
                 SipUserAgent& rSipUserAgent,
                 CpMediaInterfaceFactory& rMediaInterfaceFactory,
                 const SdpCodecList& rDefaultSdpCodecList,
                 OsMsgQ& rCallManagerQueue,
                 const CpNatTraversalConfig& rNatTraversalConfig,
                 const UtlString& sLocalIpAddress,
                 int inviteExpireSeconds,
                 XCpCallConnectionListener* pCallConnectionListener,
                 CpCallStateEventListener* pCallEventListener,
                 SipInfoStatusEventListener* pInfoStatusEventListener,
                 SipSecurityEventListener* pSecurityEventListener,
                 CpMediaEventListener* pMediaEventListener)
: XCpAbstractCall(sId, rSipUserAgent, rMediaInterfaceFactory, rDefaultSdpCodecList, rCallManagerQueue, rNatTraversalConfig,
                  sLocalIpAddress, inviteExpireSeconds, pCallConnectionListener, pCallEventListener, pInfoStatusEventListener,
                  pSecurityEventListener, pMediaEventListener)
, m_pSipConnection(NULL)
{

}

XCpCall::~XCpCall()
{
   destroySipConnection(); // destroy connection if it still exists
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */

OsStatus XCpCall::connect(const UtlString& sipCallId,
                          SipDialog& sipDialog,
                          const UtlString& toAddress,
                          const UtlString& fromAddress,
                          const UtlString& locationHeader,
                          CP_CONTACT_ID contactId)
{
   if (sipCallId.isNull() || toAddress.isNull() || fromAddress.isNull())
   {
      return OS_FAILED;
   }

   UtlString localTag(m_sipTagGenerator.getNewTag());
   sipDialog = SipDialog(sipCallId, localTag, NULL);

   AcConnectMsg connectMsg(sipCallId, toAddress, localTag, fromAddress, locationHeader, contactId);
   return postMessage(connectMsg);
}

OsStatus XCpCall::acceptConnection(const UtlString& locationHeader,
                                   CP_CONTACT_ID contactId)
{
   AcAcceptConnectionMsg acceptConnectionMsg(locationHeader, contactId);
   return postMessage(acceptConnectionMsg);
}

OsStatus XCpCall::rejectConnection()
{
   AcRejectConnectionMsg rejectConnectionMsg;
   return postMessage(rejectConnectionMsg);
}

OsStatus XCpCall::redirectConnection(const UtlString& sRedirectSipUrl)
{
   AcRedirectConnectionMsg redirectConnectionMsg(sRedirectSipUrl);
   return postMessage(redirectConnectionMsg);
}

OsStatus XCpCall::answerConnection()
{
   AcAnswerConnectionMsg answerConnectionMsg;
   return postMessage(answerConnectionMsg);
}

OsStatus XCpCall::dropConnection(const SipDialog& sipDialog, UtlBoolean bDestroyCall)
{
   AcDropConnectionMsg dropConnectionMsg(sipDialog, bDestroyCall);
   return postMessage(dropConnectionMsg);
}

OsStatus XCpCall::dropConnection(UtlBoolean bDestroyCall)
{
   AcDropConnectionMsg dropConnectionMsg(NULL, bDestroyCall);
   return postMessage(dropConnectionMsg);
}

OsStatus XCpCall::transferBlind(const SipDialog& sipDialog,
                                const UtlString& sTransferSipUrl)
{
   AcTransferBlindMsg transferBlindMsg(sipDialog, sTransferSipUrl);
   return postMessage(transferBlindMsg);
}

OsStatus XCpCall::holdConnection(const SipDialog& sipDialog)
{
   AcHoldConnectionMsg holdConnectionMsg(sipDialog);
   return postMessage(holdConnectionMsg);
}

OsStatus XCpCall::holdConnection()
{
   AcHoldConnectionMsg holdConnectionMsg(NULL);
   return postMessage(holdConnectionMsg);
}

OsStatus XCpCall::unholdConnection(const SipDialog& sipDialog)
{
   AcUnholdConnectionMsg unholdConnectionMsg(sipDialog);
   return postMessage(unholdConnectionMsg);
}

OsStatus XCpCall::unholdConnection()
{
   AcUnholdConnectionMsg unholdConnectionMsg(NULL);
   return postMessage(unholdConnectionMsg);
}

OsStatus XCpCall::renegotiateCodecsConnection(const SipDialog& sipDialog,
                                              const UtlString& sAudioCodecs,
                                              const UtlString& sVideoCodecs)
{
   AcRenegotiateCodecsMsg renegotiateCodecsMsg(sipDialog, sAudioCodecs,
      sVideoCodecs);
   return postMessage(renegotiateCodecsMsg);
}

OsStatus XCpCall::sendInfo(const SipDialog& sipDialog,
                           const UtlString& sContentType,
                           const char* pContent,
                           const size_t nContentLength)
{
   AcSendInfoMsg sendInfoMsg(sipDialog, sContentType, pContent, nContentLength);
   return postMessage(sendInfoMsg);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

SipDialog::DialogMatchEnum XCpCall::hasSipDialog(const SipDialog& sipDialog) const
{
   OsLock lock(m_memberMutex);

   if (m_pSipConnection)
   {
      return m_pSipConnection->compareSipDialog(sipDialog);
   }

   return SipDialog::DIALOG_MISMATCH;
}

int XCpCall::getCallCount() const
{
   return m_pSipConnection != NULL ? 1 : 0;
}

OsStatus XCpCall::getCallSipCallId(UtlString& sSipCallId) const
{
   OsStatus result = OS_INVALID;

   OsPtrLock<XSipConnection> ptrLock; // auto pointer lock
   UtlBoolean resFind = getConnection(ptrLock);
   if (resFind)
   {
      ptrLock->getSipCallId(sSipCallId);
      result = OS_SUCCESS;
   }

   return result;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

UtlBoolean XCpCall::findConnection(const SipDialog& sipDialog, OsPtrLock<XSipConnection>& ptrLock) const
{
   OsLock lock(m_memberMutex);

   if (m_pSipConnection && m_pSipConnection->compareSipDialog(sipDialog) != SipDialog::DIALOG_MISMATCH)
   {
      // dialog matches
      ptrLock = m_pSipConnection;
      return TRUE;
   }

   return FALSE;
}

UtlBoolean XCpCall::getConnection(OsPtrLock<XSipConnection>& ptrLock) const
{
   OsLock lock(m_memberMutex);

   ptrLock = m_pSipConnection;
   return m_pSipConnection != NULL;
}

UtlBoolean XCpCall::handleCommandMessage(const AcCommandMsg& rRawMsg)
{
   switch ((AcCommandMsg::SubTypeEnum)rRawMsg.getMsgSubType())
   {
   case AcCommandMsg::AC_CONNECT:
      handleConnect((const AcConnectMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_ACCEPT_CONNECTION:
      handleAcceptConnection((const AcAcceptConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_REJECT_CONNECTION:
      handleRejectConnection((const AcRejectConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_REDIRECT_CONNECTION:
      handleRedirectConnection((const AcRedirectConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_ANSWER_CONNECTION:
      handleAnswerConnection((const AcAnswerConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_DROP_CONNECTION:
      handleDropConnection((const AcDropConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_TRANSFER_BLIND:
      handleTransferBlind((const AcTransferBlindMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_HOLD_CONNECTION:
      handleHoldConnection((const AcHoldConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_UNHOLD_CONNECTION:
      handleUnholdConnection((const AcUnholdConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_RENEGOTIATE_CODECS:
      handleRenegotiateCodecs((const AcRenegotiateCodecsMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_SEND_INFO:
      handleSendInfo((const AcSendInfoMsg&)rRawMsg);
      return TRUE;
   default:
      break;
   }

   // we couldn't handle it, give chance to parent
   return XCpAbstractCall::handleCommandMessage(rRawMsg);
}

UtlBoolean XCpCall::handleNotificationMessage(const AcNotificationMsg& rRawMsg)
{
   // we couldn't handle it, give chance to parent
   return XCpAbstractCall::handleNotificationMessage(rRawMsg);
}

UtlBoolean XCpCall::handleTimerMessage(const CpTimerMsg& rRawMsg)
{
   // we couldn't handle it, give chance to parent
   return XCpAbstractCall::handleTimerMessage(rRawMsg);
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

OsStatus XCpCall::handleConnect(const AcConnectMsg& rMsg)
{
   OsStatus result = OS_FAILED;
   // thread safe check, as connection is only created/destroyed in this thread
   if (!m_pSipConnection)
   {
      SipDialog sipDialog(rMsg.getSipCallId(), rMsg.getLocalTag(), NULL, TRUE);
      createSipConnection(sipDialog);
   }

   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = getConnection(ptrLock);
   if (resFound)
   {
      result = ptrLock->connect(rMsg.getSipCallId(), rMsg.getLocalTag(), rMsg.getToAddress(), rMsg.getFromAddress(),
         rMsg.getLocationHeader(), rMsg.getContactId());
   }

   return result;
}

OsStatus XCpCall::handleAcceptConnection(const AcAcceptConnectionMsg& rMsg)
{
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = getConnection(ptrLock);
   if (resFound)
   {
      return ptrLock->acceptConnection(rMsg.getLocationHeader(), rMsg.getContactId());
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleRejectConnection(const AcRejectConnectionMsg& rMsg)
{
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = getConnection(ptrLock);
   if (resFound)
   {
      return ptrLock->rejectConnection();
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleRedirectConnection(const AcRedirectConnectionMsg& rMsg)
{
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = getConnection(ptrLock);
   if (resFound)
   {
      return ptrLock->redirectConnection(rMsg.getRedirectSipUrl());
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleAnswerConnection(const AcAnswerConnectionMsg& rMsg)
{
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = getConnection(ptrLock);
   if (resFound)
   {
      return ptrLock->answerConnection();
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleDropConnection(const AcDropConnectionMsg& rMsg)
{
   SipDialog sipDialog;
   rMsg.getSipDialog(sipDialog);
   UtlBoolean resFound = FALSE;
   // find connection by sip dialog if call-id is not null
   OsPtrLock<XSipConnection> ptrLock;
   if (sipDialog.getCallId().isNull())
   {
      resFound = getConnection(ptrLock);
   }
   else
   {
      resFound = findConnection(sipDialog, ptrLock);
   }
   if (resFound)
   {
      return ptrLock->dropConnection(rMsg.getDestroyAbstractCall());
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleTransferBlind(const AcTransferBlindMsg& rMsg)
{
   SipDialog sipDialog;
   rMsg.getSipDialog(sipDialog);
   // find connection by sip dialog
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = findConnection(sipDialog, ptrLock);
   if (resFound)
   {
      return ptrLock->transferBlind(rMsg.getTransferSipUrl());
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleHoldConnection(const AcHoldConnectionMsg& rMsg)
{
   SipDialog sipDialog;
   rMsg.getSipDialog(sipDialog);
   UtlBoolean resFound = FALSE;
   // find connection by sip dialog if call-id is not null
   OsPtrLock<XSipConnection> ptrLock;
   if (sipDialog.getCallId().isNull())
   {
      resFound = getConnection(ptrLock);
   }
   else
   {
      resFound = findConnection(sipDialog, ptrLock);
   }
   if (resFound)
   {
      return ptrLock->holdConnection();
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleUnholdConnection(const AcUnholdConnectionMsg& rMsg)
{
   SipDialog sipDialog;
   rMsg.getSipDialog(sipDialog);
   UtlBoolean resFound = FALSE;
   // find connection by sip dialog if call-id is not null
   OsPtrLock<XSipConnection> ptrLock;
   if (sipDialog.getCallId().isNull())
   {
      resFound = getConnection(ptrLock);
   }
   else
   {
      resFound = findConnection(sipDialog, ptrLock);
   }
   if (resFound)
   {
      return ptrLock->unholdConnection();
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleRenegotiateCodecs(const AcRenegotiateCodecsMsg& rMsg)
{
   SipDialog sipDialog;
   rMsg.getSipDialog(sipDialog);
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = findConnection(sipDialog, ptrLock);
   if (resFound)
   {
      return ptrLock->renegotiateCodecsConnection(rMsg.getAudioCodecs(), rMsg.getVideoCodecs());
   }

   return OS_NOT_FOUND;
}

OsStatus XCpCall::handleSendInfo(const AcSendInfoMsg& rMsg)
{
   SipDialog sipDialog;
   rMsg.getSipDialog(sipDialog);
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = findConnection(sipDialog, ptrLock);
   if (resFound)
   {
      return ptrLock->sendInfo(rMsg.getContentType(), rMsg.getContent(), rMsg.getContentLength());
   }

   return OS_NOT_FOUND;
}

void XCpCall::createSipConnection(const SipDialog& sipDialog)
{
   UtlBoolean bAdded = FALSE;
   {
      OsLock lock(m_memberMutex);
      if (!m_pSipConnection)
      {
         m_pSipConnection = new XSipConnection(m_sId, sipDialog, m_rSipUserAgent, m_inviteExpireSeconds, *this, *this, m_natTraversalConfig,
            m_pCallEventListener, m_pInfoStatusEventListener, m_pSecurityEventListener, m_pMediaEventListener);
         bAdded = TRUE;
      }
   }

   if (bAdded)
   {
      onConnectionAddded(sipDialog.getCallId());
   }
}

void XCpCall::destroySipConnection()
{
   UtlString sSipCallId;
   {
      OsLock lock(m_memberMutex);
      if (m_pSipConnection)
      {
         m_pSipConnection->getSipCallId(sSipCallId);
         delete m_pSipConnection;
         m_pSipConnection = NULL;
      }
   }

   if (!sSipCallId.isNull())
   {
      // we destroyed some connection, notify call stack
      onConnectionRemoved(sSipCallId);
   }
}

void XCpCall::fireSipXMediaConnectionEvent(CP_MEDIA_EVENT event,
                                           CP_MEDIA_CAUSE cause,
                                           CP_MEDIA_TYPE type,
                                           int mediaConnectionId,
                                           intptr_t pEventData1,
                                           intptr_t pEventData2)
{
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = getConnection(ptrLock);
   if (resFound)
   {
      if (ptrLock->getMediaConnectionId() == mediaConnectionId)
      {
         ptrLock->handleSipXMediaEvent(event, cause, type, pEventData1, pEventData2);
      }
   }
}

void XCpCall::fireSipXMediaInterfaceEvent(CP_MEDIA_EVENT event, 
                                          CP_MEDIA_CAUSE cause,
                                          CP_MEDIA_TYPE type,
                                          intptr_t pEventData1,
                                          intptr_t pEventData2)
{
   OsPtrLock<XSipConnection> ptrLock;
   UtlBoolean resFound = getConnection(ptrLock);
   if (resFound)
   {
      ptrLock->handleSipXMediaEvent(event, cause, type, pEventData1, pEventData2);
   }
}

/* ============================ FUNCTIONS ================================= */
