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
#include <utl/UtlSListIterator.h>
#include <net/SipDialog.h>
#include <cp/XCpConference.h>
#include <cp/XSipConnection.h>
#include <cp/msg/AcConnectMsg.h>
#include <cp/msg/AcDropConnectionMsg.h>
#include <cp/msg/AcDropAllConnectionsMsg.h>
#include <cp/msg/AcHoldConnectionMsg.h>
#include <cp/msg/AcHoldAllConnectionsMsg.h>
#include <cp/msg/AcUnholdConnectionMsg.h>
#include <cp/msg/AcUnholdAllConnectionsMsg.h>
#include <cp/msg/AcTransferBlindMsg.h>
#include <cp/msg/AcLimitCodecPreferencesMsg.h>
#include <cp/msg/AcRenegotiateCodecsMsg.h>
#include <cp/msg/AcRenegotiateCodecsAllMsg.h>
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

XCpConference::XCpConference(const UtlString& sId,
                             SipUserAgent& rSipUserAgent,
                             CpMediaInterfaceFactory& rMediaInterfaceFactory,
                             OsMsgQ& rCallManagerQueue)
: XCpAbstractCall(sId, rSipUserAgent, rMediaInterfaceFactory, rCallManagerQueue)
{

}

XCpConference::~XCpConference()
{
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */

OsStatus XCpConference::connect(const UtlString& sipCallId,
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

OsStatus XCpConference::acceptConnection(const UtlString& locationHeader,
                                         CP_CONTACT_ID contactId)
{
   // conference cannot have new inbound call
   return OS_NOT_SUPPORTED;
}

OsStatus XCpConference::rejectConnection()
{
   // conference cannot have new inbound call
   return OS_NOT_SUPPORTED;
}

OsStatus XCpConference::redirectConnection(const UtlString& sRedirectSipUrl)
{
   // conference cannot have new inbound call
   return OS_NOT_SUPPORTED;
}

OsStatus XCpConference::answerConnection()
{
   // conference cannot have new inbound call
   return OS_NOT_SUPPORTED;
}

OsStatus XCpConference::dropConnection(const SipDialog& sipDialog, UtlBoolean bDestroyConference)
{
   AcDropConnectionMsg dropConnectionMsg(sipDialog, bDestroyConference);
   return postMessage(dropConnectionMsg);
}

OsStatus XCpConference::dropAllConnections(UtlBoolean bDestroyConference)
{
   AcDropAllConnectionsMsg dropAllConnectionsMsg(bDestroyConference);
   return postMessage(dropAllConnectionsMsg);
}

OsStatus XCpConference::transferBlind(const SipDialog& sipDialog,
                                      const UtlString& sTransferSipUrl)
{
   AcTransferBlindMsg transferBlindMsg(sipDialog, sTransferSipUrl);
   return postMessage(transferBlindMsg);
}

OsStatus XCpConference::holdConnection(const SipDialog& sipDialog)
{
   AcHoldConnectionMsg holdConnectionMsg(sipDialog);
   return postMessage(holdConnectionMsg);
}

OsStatus XCpConference::holdAllConnections()
{
   AcHoldAllConnectionsMsg holdAllConnectionsMsg;
   return postMessage(holdAllConnectionsMsg);
}

OsStatus XCpConference::unholdConnection(const SipDialog& sipDialog)
{
   AcUnholdConnectionMsg unholdConnectionMsg(sipDialog);
   return postMessage(unholdConnectionMsg);
}

OsStatus XCpConference::unholdAllConnections()
{
   AcUnholdAllConnectionsMsg unholdAllConnectionsMsg;
   return postMessage(unholdAllConnectionsMsg);
}

OsStatus XCpConference::muteInputConnection(const SipDialog& sipDialog)
{
   AcMuteInputConnectionMsg muteInputConnectionMsg(sipDialog);
   return postMessage(muteInputConnectionMsg);
}

OsStatus XCpConference::unmuteInputConnection(const SipDialog& sipDialog)
{
   AcUnmuteInputConnectionMsg unmuteInputConnectionMsg(sipDialog);
   return postMessage(unmuteInputConnectionMsg);
}

OsStatus XCpConference::limitCodecPreferences(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                              const UtlString& sAudioCodecs,
                                              CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                              const UtlString& sVideoCodecs)
{
   AcLimitCodecPreferencesMsg limitCodecPreferencesMsg(audioBandwidthId, sAudioCodecs,
      videoBandwidthId, sVideoCodecs);
   return postMessage(limitCodecPreferencesMsg);
}

OsStatus XCpConference::renegotiateCodecsConnection(const SipDialog& sipDialog,
                                                    CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                    const UtlString& sAudioCodecs,
                                                    CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                    const UtlString& sVideoCodecs)
{
   AcRenegotiateCodecsMsg renegotiateCodecsMsg(sipDialog, audioBandwidthId, sAudioCodecs,
      videoBandwidthId, sVideoCodecs);
   return postMessage(renegotiateCodecsMsg);
}

OsStatus XCpConference::renegotiateCodecsAllConnections(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                        const UtlString& sAudioCodecs,
                                                        CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                        const UtlString& sVideoCodecs)
{
   AcRenegotiateCodecsAllMsg renegotiateCodecsMsg(audioBandwidthId, sAudioCodecs,
      videoBandwidthId, sVideoCodecs);
   return postMessage(renegotiateCodecsMsg);
}

OsStatus XCpConference::sendInfo(const SipDialog& sipDialog,
                                 const UtlString& sContentType,
                                 const char* pContent,
                                 const size_t nContentLength)
{
   AcSendInfoMsg sendInfoMsg(sipDialog, sContentType, pContent, nContentLength);
   return postMessage(sendInfoMsg);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

SipDialog::DialogMatchEnum XCpConference::hasSipDialog(const SipDialog& sipDialog) const
{
   SipDialog::DialogMatchEnum result = SipDialog::DIALOG_MISMATCH;

   OsLock lock(m_memberMutex);

   UtlSListIterator itor(m_sipConnections);
   XSipConnection* pSipConnection = NULL;

   while (itor())
   {
      pSipConnection = dynamic_cast<XSipConnection*>(itor.item());
      if (pSipConnection)
      {
         SipDialog::DialogMatchEnum tmpResult = pSipConnection->compareSipDialog(sipDialog);
         if (tmpResult == SipDialog::DIALOG_ESTABLISHED_MATCH ||
             (sipDialog.isInitialDialog() && tmpResult == SipDialog::DIALOG_INITIAL_MATCH))
         {
            // return immediately if found perfect match for established dialog
            // initial match is also perfect if supplied dialog is initial
            return tmpResult;
         }
         else if (tmpResult != SipDialog::DIALOG_MISMATCH)
         {
            // only override result if we found some match, as we could have some connection at the end of list that would not match
            result = tmpResult;
         }
      }
   }

   return result;
}

int XCpConference::getCallCount() const
{
   // thread safe
   return (int)m_sipConnections.entries();
}

OsStatus XCpConference::getConferenceSipCallIds(UtlSList& sipCallIdList) const
{
   sipCallIdList.destroyAll();

   OsLock lock(m_memberMutex);

   UtlSListIterator itor(m_sipConnections);
   XSipConnection* pSipConnection = NULL;
   UtlString sipCallId;

   while (itor())
   {
      pSipConnection = dynamic_cast<XSipConnection*>(itor.item());
      if (pSipConnection)
      {
         pSipConnection->getSipCallId(sipCallId);
         if (!sipCallId.isNull())
         {
            sipCallIdList.append(sipCallId.clone());
         }
      }
   }

   return OS_SUCCESS;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

UtlBoolean XCpConference::findConnection(const SipDialog& sipDialog, OsPtrLock<XSipConnection>& ptrLock) const
{
   OsLock lock(m_memberMutex);

   UtlSListIterator itor(m_sipConnections);
   XSipConnection* pSipConnection = NULL;

   while (itor())
   {
      pSipConnection = dynamic_cast<XSipConnection*>(itor.item());
      if (pSipConnection && pSipConnection->compareSipDialog(sipDialog) != SipDialog::DIALOG_MISMATCH)
      {
         ptrLock = pSipConnection;
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean XCpConference::handleCommandMessage(AcCommandMsg& rRawMsg)
{
   switch ((AcCommandMsg::SubTypesEnum)rRawMsg.getMsgSubType())
   {
   case AcCommandMsg::AC_CONNECT:
      return TRUE;
   case AcCommandMsg::AC_DROP_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_DROP_ALL_CONNECTIONS:
      return TRUE;
   case AcCommandMsg::AC_TRANSFER_BLIND:
      return TRUE;
   case AcCommandMsg::AC_HOLD_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_HOLD_ALL_CONNECTIONS:
      return TRUE;
   case AcCommandMsg::AC_UNHOLD_CONNECTION:
      return TRUE;
   case AcCommandMsg::AC_UNHOLD_ALL_CONNECTIONS:
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

UtlBoolean XCpConference::handleNotificationMessage(AcNotificationMsg& rRawMsg)
{
   // we couldn't handle it, give chance to parent
   return XCpAbstractCall::handleNotificationMessage(rRawMsg);
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
