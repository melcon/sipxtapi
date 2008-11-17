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
#include <cp/msg/AcRenegotiateCodecsMsg.h>
#include <cp/msg/AcRenegotiateCodecsAllMsg.h>
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

XCpConference::XCpConference(const UtlString& sId,
                             SipUserAgent& rSipUserAgent,
                             CpMediaInterfaceFactory& rMediaInterfaceFactory,
                             const SdpCodecList& rDefaultSdpCodecList,
                             OsMsgQ& rCallManagerQueue,
                             CpCallStateEventListener* pCallEventListener,
                             SipInfoStatusEventListener* pInfoStatusEventListener,
                             SipSecurityEventListener* pSecurityEventListener,
                             CpMediaEventListener* pMediaEventListener)
: XCpAbstractCall(sId, rSipUserAgent, rMediaInterfaceFactory, rDefaultSdpCodecList, rCallManagerQueue,
                  pCallEventListener, pInfoStatusEventListener, pSecurityEventListener, pMediaEventListener)
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

OsStatus XCpConference::renegotiateCodecsConnection(const SipDialog& sipDialog,
                                                    const UtlString& sAudioCodecs,
                                                    const UtlString& sVideoCodecs)
{
   AcRenegotiateCodecsMsg renegotiateCodecsMsg(sipDialog, sAudioCodecs,
      sVideoCodecs);
   return postMessage(renegotiateCodecsMsg);
}

OsStatus XCpConference::renegotiateCodecsAllConnections(const UtlString& sAudioCodecs,
                                                        const UtlString& sVideoCodecs)
{
   AcRenegotiateCodecsAllMsg renegotiateCodecsMsg(sAudioCodecs, sVideoCodecs);
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
             tmpResult == SipDialog::DIALOG_INITIAL_INITIAL_MATCH)
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
   XSipConnection* pPartialMatchSipConnection = NULL;
   XSipConnection* pSipConnection = NULL;

   while (itor())
   {
      pSipConnection = dynamic_cast<XSipConnection*>(itor.item());
      if (pSipConnection)
      {
         SipDialog::DialogMatchEnum tmpResult = pSipConnection->compareSipDialog(sipDialog);
         if (tmpResult == SipDialog::DIALOG_ESTABLISHED_MATCH ||
             tmpResult == SipDialog::DIALOG_INITIAL_INITIAL_MATCH)
         {
            // return immediately if found perfect match for established dialog
            // initial match is also perfect if supplied dialog is initial
            ptrLock = pSipConnection;
            return TRUE;
         }
         else if (tmpResult != SipDialog::DIALOG_MISMATCH)
         {
            // only override result if we found some match, as we could have some connection at the end of list that would not match
            pPartialMatchSipConnection = pSipConnection;
         }
      }
   }

   if (pPartialMatchSipConnection)
   {
      // return partial match at the end
      ptrLock = pPartialMatchSipConnection;
      return TRUE;
   }

   // not even partial match was found
   return FALSE;
}

UtlBoolean XCpConference::handleCommandMessage(const AcCommandMsg& rRawMsg)
{
   switch ((AcCommandMsg::SubTypeEnum)rRawMsg.getMsgSubType())
   {
   case AcCommandMsg::AC_CONNECT:
      handleConnect((const AcConnectMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_DROP_CONNECTION:
      handleDropConnection((const AcDropConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_DROP_ALL_CONNECTIONS:
      handleDropAllConnections((const AcDropAllConnectionsMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_TRANSFER_BLIND:
      handleTransferBlind((const AcTransferBlindMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_HOLD_CONNECTION:
      handleHoldConnection((const AcHoldConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_HOLD_ALL_CONNECTIONS:
      handleHoldAllConnections((const AcHoldAllConnectionsMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_UNHOLD_CONNECTION:
      handleUnholdConnection((const AcUnholdConnectionMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_UNHOLD_ALL_CONNECTIONS:
      handleUnholdAllConnections((const AcUnholdAllConnectionsMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_RENEGOTIATE_CODECS:
      handleRenegotiateCodecs((const AcRenegotiateCodecsMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_RENEGOTIATE_CODECS_ALL:
      handleRenegotiateCodecsAll((const AcRenegotiateCodecsAllMsg&)rRawMsg);
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

UtlBoolean XCpConference::handleNotificationMessage(const AcNotificationMsg& rRawMsg)
{
   // we couldn't handle it, give chance to parent
   return XCpAbstractCall::handleNotificationMessage(rRawMsg);
}

UtlBoolean XCpConference::handleTimerMessage(const CpTimerMsg& rRawMsg)
{
   // we couldn't handle it, give chance to parent
   return XCpAbstractCall::handleTimerMessage(rRawMsg);
}

UtlBoolean XCpConference::handleSipMessageEvent(const SipMessageEvent& rSipMsgEvent)
{
   return TRUE;
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

OsStatus XCpConference::handleConnect(const AcConnectMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleDropConnection(const AcDropConnectionMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleDropAllConnections(const AcDropAllConnectionsMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleTransferBlind(const AcTransferBlindMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleHoldConnection(const AcHoldConnectionMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleHoldAllConnections(const AcHoldAllConnectionsMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleUnholdConnection(const AcUnholdConnectionMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleUnholdAllConnections(const AcUnholdAllConnectionsMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleRenegotiateCodecs(const AcRenegotiateCodecsMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleRenegotiateCodecsAll(const AcRenegotiateCodecsAllMsg& rMsg)
{
   return OS_FAILED;
}

OsStatus XCpConference::handleSendInfo(const AcSendInfoMsg& rMsg)
{
   return OS_FAILED;
}

void XCpConference::fireSipXMediaConnectionEvent(CP_MEDIA_EVENT event,
                                                 CP_MEDIA_CAUSE cause,
                                                 CP_MEDIA_TYPE type,
                                                 int mediaConnectionId,
                                                 intptr_t pEventData1,
                                                 intptr_t pEventData2)
{
   OsLock lock(m_memberMutex);

   UtlSListIterator itor(m_sipConnections);
   XSipConnection* pSipConnection = NULL;

   while (itor())
   {
      pSipConnection = dynamic_cast<XSipConnection*>(itor.item());
      if (pSipConnection && pSipConnection->getMediaConnectionId() == mediaConnectionId)
      {
         pSipConnection->handleSipXMediaEvent(event, cause, type, pEventData1, pEventData2);
      }
   }
}

void XCpConference::fireSipXMediaInterfaceEvent(CP_MEDIA_EVENT event,
                                                CP_MEDIA_CAUSE cause,
                                                CP_MEDIA_TYPE type,
                                                intptr_t pEventData1,
                                                intptr_t pEventData2)
{
   OsLock lock(m_memberMutex);

   UtlSListIterator itor(m_sipConnections);
   XSipConnection* pSipConnection = NULL;

   while (itor())
   {
      pSipConnection = dynamic_cast<XSipConnection*>(itor.item());
      if (pSipConnection)
      {
         pSipConnection->handleSipXMediaEvent(event, cause, type, pEventData1, pEventData2);
      }
   }
}

/* ============================ FUNCTIONS ================================= */
