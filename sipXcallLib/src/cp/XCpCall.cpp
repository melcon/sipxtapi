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
#include <cp/msg/AcLimitCodecPreferencesMsg.h>
#include <cp/msg/AcRenegotiateCodecsMsg.h>
#include <cp/msg/AcMuteInputConnectionMsg.h>
#include <cp/msg/AcUnmuteInputConnectionMsg.h>
#include <cp/msg/AcSendInfoMsg.h>

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
                 OsMsgQ& rCallManagerQueue)
: XCpAbstractCall(sId, rSipUserAgent, rMediaInterfaceFactory, rCallManagerQueue)
, m_pSipConnection(NULL)
{

}

XCpCall::~XCpCall()
{
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

OsStatus XCpCall::muteInputConnection(const SipDialog& sipDialog)
{
   AcMuteInputConnectionMsg muteInputConnectionMsg(sipDialog);
   return postMessage(muteInputConnectionMsg);
}

OsStatus XCpCall::unmuteInputConnection(const SipDialog& sipDialog)
{
   AcUnmuteInputConnectionMsg unmuteInputConnectionMsg(sipDialog);
   return postMessage(unmuteInputConnectionMsg);
}

OsStatus XCpCall::limitCodecPreferences(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                        const UtlString& sAudioCodecs,
                                        CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                        const UtlString& sVideoCodecs)
{
   AcLimitCodecPreferencesMsg limitCodecPreferencesMsg(audioBandwidthId, sAudioCodecs,
      videoBandwidthId, sVideoCodecs);
   return postMessage(limitCodecPreferencesMsg);
}

OsStatus XCpCall::renegotiateCodecsConnection(const SipDialog& sipDialog,
                                              CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                              const UtlString& sAudioCodecs,
                                              CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                              const UtlString& sVideoCodecs)
{
   AcRenegotiateCodecsMsg renegotiateCodecsMsg(sipDialog, audioBandwidthId, sAudioCodecs,
      videoBandwidthId, sVideoCodecs);
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

UtlBoolean XCpCall::handleCommandMessage(AcCommandMsg& rRawMsg)
{
   switch ((AcCommandMsg::SubTypesEnum)rRawMsg.getMsgSubType())
   {
   case AcCommandMsg::AC_CONNECT:
      return TRUE;
   case AcCommandMsg::AC_ACCEPT_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_REJECT_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_REDIRECT_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_ANSWER_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_DROP_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_TRANSFER_BLIND:
      return TRUE;
   case AcCommandMsg::AC_HOLD_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_UNHOLD_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_LIMIT_CODEC_PREFERENCES:
      return TRUE;
   case AcCommandMsg::AC_RENEGOTIATE_CODECS:
      return TRUE;
   case AcCommandMsg::AC_SEND_INFO:
      return TRUE;
   case AcCommandMsg::AC_MUTE_INPUT_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_UNMUTE_INPUT_CONNECTION:
      return TRUE;
   default:
      break;
   }

   // we couldn't handle it, give chance to parent
   return XCpAbstractCall::handleCommandMessage(rRawMsg);
}

UtlBoolean XCpCall::handleNotificationMessage(AcNotificationMsg& rRawMsg)
{
   // we couldn't handle it, give chance to parent
   return XCpAbstractCall::handleNotificationMessage(rRawMsg);
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
