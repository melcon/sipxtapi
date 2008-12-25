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
#include <os/OsSysLog.h>
#include <os/OsWriteLock.h>
#include <os/OsReadLock.h>
#include <os/OsTimer.h>
#include <os/OsTimerNotification.h>
#include <sdp/SdpCodecList.h>
#include <net/SipMessage.h>
#include <net/SipUserAgent.h>
#include <mi/CpMediaInterface.h>
#include <cp/CpMessageQueueProvider.h>
#include <cp/XSipConnectionContext.h>
#include <cp/state/BaseSipConnectionState.h>
#include <cp/state/SipConnectionStateTransition.h>
#include <cp/state/StateTransitionMemory.h>
#include <cp/state/SipResponseTransitionMemory.h>
#include <cp/state/SipEventTransitionMemory.h>
#include <cp/state/GeneralTransitionMemory.h>
#include <cp/CpMediaInterfaceProvider.h>
#include <cp/msg/ScTimerMsg.h>
#include <cp/msg/ScCommandMsg.h>
#include <cp/msg/ScNotificationMsg.h>
#include <cp/msg/Sc100RelTimerMsg.h>
#include <cp/msg/Sc2xxTimerMsg.h>
#include <cp/msg/ScDisconnectTimerMsg.h>
#include <cp/msg/ScReInviteTimerMsg.h>
#include <cp/msg/ScByeRetryTimerMsg.h>
#include <cp/msg/AcDestroyConnectionMsg.h>
#include <cp/XSipConnectionEventSink.h>

// DEFINES
// CANCEL doesn't need transaction tracking
#define TRACKABLE_METHODS "INVITE UPDATE INFO NOTIFY REFER OPTIONS PRACK SUBSCRIBE BYE"
#define MAX_RENEGOTIATION_RETRY_COUNT 10

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
int BaseSipConnectionState::ms_iInfoTestResponseCode = 0;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

BaseSipConnectionState::BaseSipConnectionState(SipConnectionStateContext& rStateContext,
                                               SipUserAgent& rSipUserAgent,
                                               CpMediaInterfaceProvider& rMediaInterfaceProvider,
                                               CpMessageQueueProvider& rMessageQueueProvider,
                                               XSipConnectionEventSink& rSipConnectionEventSink,
                                               const CpNatTraversalConfig& natTraversalConfig)
: m_rStateContext(rStateContext)
, m_rSipUserAgent(rSipUserAgent)
, m_rMediaInterfaceProvider(rMediaInterfaceProvider)
, m_rMessageQueueProvider(rMessageQueueProvider)
, m_rSipConnectionEventSink(rSipConnectionEventSink)
, m_natTraversalConfig(natTraversalConfig)
{

}

BaseSipConnectionState::BaseSipConnectionState(const BaseSipConnectionState& rhs)
: m_rStateContext(rhs.m_rStateContext)
, m_rSipUserAgent(rhs.m_rSipUserAgent)
, m_rMediaInterfaceProvider(rhs.m_rMediaInterfaceProvider)
, m_rMessageQueueProvider(rhs.m_rMessageQueueProvider)
, m_rSipConnectionEventSink(rhs.m_rSipConnectionEventSink)
, m_natTraversalConfig(rhs.m_natTraversalConfig)
{

}

BaseSipConnectionState::~BaseSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void BaseSipConnectionState::handleStateEntry(StateEnum previousState, const StateTransitionMemory* pTransitionMemory)
{
}

void BaseSipConnectionState::handleStateExit(StateEnum nextState, const StateTransitionMemory* pTransitionMemory)
{
}

SipConnectionStateTransition* BaseSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   const SipMessage* pSipMessage = rEvent.getMessage();
   int messageType = rEvent.getMessageStatus();

   switch(messageType)
   {
      case SipMessageEvent::TRANSPORT_ERROR:
         {
            // drop the whole dialog
            getClientTransactionManager().endInviteTransaction();
            SipEventTransitionMemory memory(messageType);
            return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
         }
      case SipMessageEvent::AUTHENTICATION_RETRY:
         {
            if (pSipMessage)
            {
               return handleAuthenticationRetryEvent(*pSipMessage);
            }
            break;
         }
      case SipMessageEvent::APPLICATION:
         {
            // normal sip message received
            if (pSipMessage)
            {
               return processSipMessage(*pSipMessage);
            }
            break;
         }
      default:
         // ignore, not interesting for us
         ;
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::connect(OsStatus& result,
                                                              const UtlString& sipCallId,
                                                              const UtlString& localTag,
                                                              const UtlString& toAddress,
                                                              const UtlString& fromAddress,
                                                              const UtlString& locationHeader,
                                                              CP_CONTACT_ID contactId)
{
   // we reject connect in all states except for Dialing
   result = OS_FAILED;
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::dropConnection(OsStatus& result)
{
   // implemented in subclasses
   result = OS_FAILED;
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::holdConnection(OsStatus& result)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState == ISipConnectionState::CONNECTION_ESTABLISHED)
   {
      if (mayRenegotiateMediaSession())
      {
         // no invite transaction, we may start hold immediately
         doHold();
         result = OS_SUCCESS;
         return NULL;
      }
      else // some invite is active
      {
         if (m_rStateContext.m_pSessionRenegotiationTimer == NULL)
         {
            // timer is not started yet, schedule hold
            startSessionRenegotiationTimer(ScReInviteTimerMsg::REASON_HOLD, TRUE);
            result = OS_SUCCESS;
            return NULL;
         } // else fail
      }
   }

   result = OS_FAILED;
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::unholdConnection(OsStatus& result)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState == ISipConnectionState::CONNECTION_ESTABLISHED)
   {
      if (mayRenegotiateMediaSession())
      {
         // no invite transaction, we may start unhold immediately
         doUnhold();
         result = OS_SUCCESS;
         return NULL;
      }
      else // some invite is active
      {
         if (m_rStateContext.m_pSessionRenegotiationTimer == NULL)
         {
            // timer is not started yet, schedule unhold
            startSessionRenegotiationTimer(ScReInviteTimerMsg::REASON_UNHOLD, TRUE);
            result = OS_SUCCESS;
            return NULL;
         } // else fail
      }
   }

   result = OS_FAILED;
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::renegotiateCodecsConnection(OsStatus& result)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState == ISipConnectionState::CONNECTION_ESTABLISHED)
   {
      if (mayRenegotiateMediaSession())
      {
         // no invite transaction, we may renegotiate immediately
         renegotiateMediaSession();
         result = OS_SUCCESS;
         return NULL;
      }
      else // some invite/update is active
      {
         if (m_rStateContext.m_pSessionRenegotiationTimer == NULL)
         {
            // timer is not started yet, schedule renegotiation
            startSessionRenegotiationTimer(ScReInviteTimerMsg::REASON_NORMAL, TRUE);
            result = OS_SUCCESS;
            return NULL;
         } // else fail
      }
   }

   result = OS_FAILED;
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::sendInfo(OsStatus& result,
                                                               const UtlString& sContentType,
                                                               const char* pContent,
                                                               const size_t nContentLength,
                                                               void* pCookie)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED &&
      connectionState != ISipConnectionState::CONNECTION_UNKNOWN)
   {
      SipDialog::DialogState dialogState;
      {
         OsReadLock lock(m_rStateContext);
         dialogState = m_rStateContext.m_sipDialog.getDialogState();
      }
      if (dialogState == SipDialog::DIALOG_STATE_ESTABLISHED)
      {
         // dialog is established (early or confirmed), we may send info
         int iCSeq = getNextLocalCSeq();
         // start transaction manually because we need to store custom data in it
         getClientTransactionManager().startTransaction(SIP_INFO_METHOD, iCSeq);
         getClientTransactionManager().setTransactionData(SIP_INFO_METHOD, iCSeq, pCookie); // assign cookie to transaction
         SipMessage infoRequest;
         prepareSipRequest(infoRequest, SIP_INFO_METHOD, iCSeq);

         if (!sContentType.isNull())
         {
            infoRequest.setContentType(sContentType);
         }
         if (nContentLength > 0)
         {
            // set INFO payload
            infoRequest.setContentLength(nContentLength);
            HttpBody* pBody = new HttpBody(pContent, nContentLength);
            infoRequest.setBody(pBody);
         }

         // try to send
         if (sendMessage(infoRequest))
         {
            result = OS_SUCCESS;
            return NULL;
         }
         else
         {
            m_rSipConnectionEventSink.fireSipXInfoStatusEvent(CP_INFOSTATUS_NETWORK_ERROR, SIPXTACK_MESSAGE_FAILURE,
               NULL, 0, pCookie);
         }
      }
   }

   result = OS_FAILED;
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleTimerMessage(const ScTimerMsg& timerMsg)
{
   switch (timerMsg.getPayloadType())
   {
   case ScTimerMsg::PAYLOAD_TYPE_100REL:
      return handle100RelTimerMessage((const Sc100RelTimerMsg&)timerMsg);
   case ScTimerMsg::PAYLOAD_TYPE_2XX:
      return handle2xxTimerMessage((const Sc2xxTimerMsg&)timerMsg);
   case ScTimerMsg::PAYLOAD_TYPE_DISCONNECT:
      return handleDisconnectTimerMessage((const ScDisconnectTimerMsg&)timerMsg);
   case ScTimerMsg::PAYLOAD_TYPE_REINVITE:
      return handleReInviteTimerMessage((const ScReInviteTimerMsg&)timerMsg);
   case ScTimerMsg::PAYLOAD_TYPE_BYE_RETRY:
      return handleByeRetryTimerMessage((const ScByeRetryTimerMsg&)timerMsg);
   default:
      ;
      OsSysLog::add(FAC_CP, PRI_WARNING, "Unknown timer message received - %d:%d\r\n",
         (int)timerMsg.getMsgSubType(), (int)timerMsg.getPayloadType());
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleCommandMessage(const ScCommandMsg& rMsg)
{
   // subclass ScCommandMsg and call specific handling function
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleNotificationMessage(const ScNotificationMsg& rMsg)
{
   // subclass ScNotificationMsg and call specific handling function
   return NULL;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

SipConnectionStateTransition* BaseSipConnectionState::getTransitionObject(BaseSipConnectionState* pDestination,
                                                                          const StateTransitionMemory* pTansitionMemory) const
{
   if (this != pDestination)
   {
      StateTransitionMemory* pTansitionMemoryCopy = NULL;
      if (pTansitionMemory)
      {
         pTansitionMemoryCopy = pTansitionMemory->clone();
      }
      // construct transition from this to destination state, containing copy of state transition memory object
      SipConnectionStateTransition* pTransition = new SipConnectionStateTransition(this, pDestination,
         pTansitionMemoryCopy);
      return pTransition;
   }
   else
   {
      return NULL;
   }
}

void BaseSipConnectionState::updateSipDialog(const SipMessage& sipMessage)
{
   OsWriteLock lock(m_rStateContext);
   m_rStateContext.m_sipDialog.updateDialog(sipMessage);
}

void BaseSipConnectionState::initializeSipDialog(const SipMessage& sipMessage)
{
   OsWriteLock lock(m_rStateContext);
   m_rStateContext.m_sipDialog = SipDialog(&sipMessage);
}

UtlBoolean BaseSipConnectionState::sendMessage(SipMessage& sipMessage)
{
   updateSipDialog(sipMessage);

   if (sipMessage.isRequest())
   {
      trackTransactionRequest(sipMessage);

      UtlString method;
      sipMessage.getRequestMethod(&method);
      if (method.compareTo(SIP_INVITE_METHOD) == 0)
      {
         // save invite message
         setLastSentInvite(sipMessage);
      }
   }
   else
   {
      trackTransactionResponse(sipMessage);
   }

   UtlBoolean res =  m_rSipUserAgent.send(sipMessage);
   if (!res)
   {
      // send failure, terminate transaction
      int seqNum;
      UtlString seqMethod;
      sipMessage.getCSeqField(&seqNum, &seqMethod);
      if (sipMessage.isRequest())
      {
         getClientTransactionManager().endTransaction(seqMethod, seqNum);
      }
      else
      {
         getServerTransactionManager().endTransaction(seqMethod, seqNum);
      }
   }

   return res;
}

SipConnectionStateTransition* BaseSipConnectionState::processSipMessage(const SipMessage& sipMessage)
{
   {
      OsWriteLock lock(m_rStateContext);
      m_rStateContext.m_sipDialog.updateDialog(sipMessage); // update dialog with received message
   }

   if (sipMessage.isRequest())
   {
      return processRequest(sipMessage);
   }
   else
   {
      return processResponse(sipMessage);
   }
}

SipConnectionStateTransition* BaseSipConnectionState::processRequest(const SipMessage& sipMessage)
{
   trackTransactionRequest(sipMessage);

   // process inbound sip message request
   UtlString method;
   sipMessage.getRequestMethod(&method);

   // update remote allow
   UtlString allowField;
   sipMessage.getAllowField(allowField);
   if (!allowField.isNull())
   {
      m_rStateContext.m_allowedRemote = allowField;
   }

   if (method.compareTo(SIP_INVITE_METHOD) == 0)
   {
      return processInviteRequest(sipMessage);
   }
   else if (method.compareTo(SIP_UPDATE_METHOD) == 0)
   {
      return processUpdateRequest(sipMessage);
   }
   else if (method.compareTo(SIP_ACK_METHOD) == 0)
   {
      return processAckRequest(sipMessage);
   }
   else if (method.compareTo(SIP_BYE_METHOD) == 0)
   {
      return processByeRequest(sipMessage);
   }
   else if (method.compareTo(SIP_CANCEL_METHOD) == 0)
   {
      return processCancelRequest(sipMessage);
   }
   else if (method.compareTo(SIP_INFO_METHOD) == 0)
   {
      return processInfoRequest(sipMessage);
   }
   else if (method.compareTo(SIP_NOTIFY_METHOD) == 0)
   {
      return processNotifyRequest(sipMessage);
   }
   else if (method.compareTo(SIP_OPTIONS_METHOD) == 0)
   {
      // TODO: currently receiving options requests is disabled in XCpCallManager. Test if in dialog OPTIONS works correctly
   }
   else if (method.compareTo(SIP_REFER_METHOD) == 0)
   {
      return processReferRequest(sipMessage);
   }
   else if (method.compareTo(SIP_SUBSCRIBE_METHOD) == 0)
   {
      // TODO: receiving in dialog subscribe is currently not enabled in XCpCallManager. Investigate
   }
   else if (method.compareTo(SIP_PRACK_METHOD) == 0)
   {
      return processPrackRequest(sipMessage);
   }
   else
   {
      // unsupported seqMethod. This can only occur if in XCpCallManager we observe sip messages that aren't supported here
      SipMessage notImplemented;
      notImplemented.setRequestUnimplemented(&sipMessage);
      m_rSipUserAgent.send(notImplemented);
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processInviteRequest(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processUpdateRequest(const SipMessage& sipMessage)
{
   int iSeqNumber = 0;
   UtlString seqMethod;
   sipMessage.getCSeqField(iSeqNumber, seqMethod);
   SipMessage sipResponse;
   UtlBoolean bProtocolError = TRUE;
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED &&
       connectionState != ISipConnectionState::CONNECTION_UNKNOWN &&
       connectionState != ISipConnectionState::CONNECTION_IDLE &&
       connectionState != ISipConnectionState::CONNECTION_DIALING &&
       connectionState != ISipConnectionState::CONNECTION_NEWCALL)
   {
      // sip dialog must be established (early or confirmed)
      // no SDP negotiation must be taking place
      SipDialog::DialogState dialogState;
      {
         OsReadLock lock(m_rStateContext);
         dialogState = m_rStateContext.m_sipDialog.getDialogState();
      }
      CpSdpNegotiation::SdpNegotiationState sdpNegotiationState = m_rStateContext.m_sdpNegotiation.getNegotiationState();

      if (dialogState == SipDialog::DIALOG_STATE_ESTABLISHED &&
          sdpNegotiationState == CpSdpNegotiation::SDP_NEGOTIATION_COMPLETE)
      {
         if (handleSdpOffer(sipMessage))
         {
            sipResponse.setOkResponseData(&sipMessage, getLocalContactUrl());
            if (prepareSdpAnswer(sipResponse))
            {
               if (commitMediaSessionChanges())
               {
                  bProtocolError = FALSE;
               }
            }
         }
      }
   }

   if (bProtocolError)
   {
      SipMessage errorResponse;
      errorResponse.setRequestBadRequest(&sipMessage);
      sendMessage(errorResponse);
   }
   else
   {
      sendMessage(sipResponse);
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processAckRequest(const SipMessage& sipMessage)
{
   UtlBoolean bProtocolError = FALSE;
   int cseqNumber = 0;
   UtlString cseqMethod;
   sipMessage.getCSeqField(cseqNumber, cseqMethod);

   if (m_rStateContext.m_pLastSent2xxToInvite)
   {
      int cseqNumber2xx = 0;
      m_rStateContext.m_pLastSent2xxToInvite->getCSeqField(&cseqNumber2xx, NULL);

      if (cseqNumber2xx == cseqNumber)
      {
         m_rStateContext.m_bAckReceived = TRUE;
         delete2xxRetransmitTimer();

         const SdpBody* pSdpBody = sipMessage.getSdpBody(m_rStateContext.m_pSecurity);
         if (pSdpBody)
         {
            if (m_rStateContext.m_sdpNegotiation.isSdpOfferFinished())
            {
               // this must be SDP answer
               if (handleSdpAnswer(sipMessage))
               {
                  commitMediaSessionChanges();
               }
               else
               {
                  bProtocolError = TRUE;
               }
            }
            else
            {
               bProtocolError = TRUE;
            }
         }
         else
         {
            // sdp bodyContent is missing, check if SDP negotiation is complete. Media session changes are already committed
            if (m_rStateContext.m_sdpNegotiation.getNegotiationState() != CpSdpNegotiation::SDP_NEGOTIATION_COMPLETE)
            {
               bProtocolError = TRUE;
            }
         }
      } // ignore bad acks
   } // ignore bad acks

   if (bProtocolError)
   {
      sendBye();
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processByeRequest(const SipMessage& sipMessage)
{
   // default handler for inbound BYE. We only allow BYE in established state.
   SipMessage sipResponse;
   // bad request, BYE is handled elsewhere
   sipResponse.setRequestBadRequest(&sipMessage);
   sendMessage(sipResponse);
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processCancelRequest(const SipMessage& sipMessage)
{
   // inbound CANCEL
   int iSeqNumber = 0;
   sipMessage.getCSeqField(&iSeqNumber, NULL); // don't use seqMethod, as it is pretty useless for us (is CANCEL)
   SipMessage sipResponse;
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED &&
      connectionState != ISipConnectionState::CONNECTION_UNKNOWN)
   {
      CpSipTransactionManager::TransactionState transactionState = getServerTransactionManager().getTransactionState(iSeqNumber);
      if (transactionState == CpSipTransactionManager::TRANSACTION_ACTIVE)
      {
         if (getServerTransactionManager().isInviteTransaction(iSeqNumber))
         {
            CpSipTransactionManager::InviteTransactionState inviteState = getServerTransactionManager().getInviteTransactionState();
            if (inviteState == CpSipTransactionManager::INITIAL_INVITE_ACTIVE)
            {
               // cancel of initial invite, disconnect call, send 487 Request terminated
               getServerTransactionManager().endTransaction(iSeqNumber);
               sipResponse.setRequestTerminatedResponseData(&sipMessage);
               sendMessage(sipResponse);
               return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, NULL);
            }
            else
            {
               // cancel of re-invite, just end transaction
               getServerTransactionManager().endTransaction(iSeqNumber);
               // send 200 OK, not possible to terminate. Has no effect. We already responded to re-INVITE
               sipResponse.setOkResponseData(&sipMessage, getLocalContactUrl());
               sendMessage(sipResponse);
            }
         }
         else
         {
            // it was some other method
            getServerTransactionManager().endTransaction(iSeqNumber);
            // send 200 OK, not possible to terminate. Has no effect.
            sipResponse.setOkResponseData(&sipMessage, getLocalContactUrl());
            sendMessage(sipResponse);
         }
      }
      else if (transactionState == CpSipTransactionManager::TRANSACTION_TERMINATED)
      {
         // send 200 OK. Has no effect.
         sipResponse.setOkResponseData(&sipMessage, getLocalContactUrl());
         sendMessage(sipResponse);
      }
      else
      {
         // transaction was not found
         sipResponse.setBadTransactionData(&sipMessage);
         sendMessage(sipResponse);

         GeneralTransitionMemory memory(CP_CALLSTATE_CAUSE_TRANSACTION_DOES_NOT_EXIST);
         return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
      }
   }
   else
   {
      // transaction was not found
      sipResponse.setBadTransactionData(&sipMessage);
      sendMessage(sipResponse);
      // we are already disconnected
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processInfoRequest(const SipMessage& sipMessage)
{
   if (ms_iInfoTestResponseCode != 0) // used in unit tests
   {
      if (ms_iInfoTestResponseCode == SIP_REQUEST_TIMEOUT_CODE)
      {
         OsTask::delay(1000);
      }
      SipMessage sipResponse;
      sipResponse.setResponseData(&sipMessage, ms_iInfoTestResponseCode, "Timed out", getLocalContactUrl());
      sendMessage(sipResponse);
      return NULL; // do not fire event if custom response code was requested.
   }
   else
   {
      // send 200 OK response regardless of content
      SipMessage sipResponse;
      sipResponse.setOkResponseData(&sipMessage, getLocalContactUrl());
      sendMessage(sipResponse);
   }

   // check for retransmits
   int cseqNum;
   UtlString cseqMethod;
   sipMessage.getCSeqField(&cseqNum, &cseqMethod);

   CpSipTransactionManager::TransactionState transactionState = getServerTransactionManager().getTransactionState(cseqMethod, cseqNum);
   if (transactionState == CpSipTransactionManager::TRANSACTION_ACTIVE)
   {
      // this is not a retransmit
      UtlString contentType;
      sipMessage.getContentType(&contentType);
      UtlString bodyContent;
      int contentLength = sipMessage.getContentLength();
      const HttpBody* pBody = sipMessage.getBody();
      if (pBody)
      {
         pBody->getBytes(&bodyContent, &contentLength);
      }
      else
      {
         contentLength = 0;
      }

      m_rSipConnectionEventSink.fireSipXInfoEvent(contentType, bodyContent.data(), contentLength);
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processNotifyRequest(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processReferRequest(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processPrackRequest(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processResponse(const SipMessage& sipMessage)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   // INVITE transaction termination is handled during disconnected state entry
   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED &&
       connectionState != ISipConnectionState::CONNECTION_UNKNOWN)
   {
      int iSeqNumber = 0;
      UtlString seqMethod;
      sipMessage.getCSeqField(iSeqNumber, seqMethod);
      int statusCode = sipMessage.getResponseStatusCode();
      UtlString statusText;
      sipMessage.getResponseStatusText(&statusText);

      CpSipTransactionManager::TransactionState transactionState = getClientTransactionManager().getTransactionState(seqMethod, iSeqNumber);
      if (transactionState == CpSipTransactionManager::TRANSACTION_ACTIVE ||
          seqMethod.compareTo(SIP_CANCEL_METHOD) == 0) // we don't track CANCEL transactions, allow it implicitly
      {
         switch (statusCode)
         {
         // these destroy the whole dialog regardless of seqMethod
         case SIP_NOT_FOUND_CODE: // 404
         case SIP_GONE_CODE: // 410
         case SIP_UNSUPPORTED_URI_SCHEME_CODE: // 416
         case SIP_LOOP_DETECTED_CODE: // 482
         case SIP_TOO_MANY_HOPS_CODE: // 483
         case SIP_BAD_ADDRESS_CODE: // 484
         case SIP_AMBIGUOUS_CODE: // 485
         case SIP_BAD_GATEWAY_CODE: // 502
         case SIP_DOESNT_EXIST_ANYWHERE_CODE: // 604
            {
               trackTransactionResponse(sipMessage);
               SipResponseTransitionMemory memory(statusCode, statusText);
               return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
            }
         // these destroy dialog usage - if seqMethod was INVITE destroy the dialog
         case SIP_BAD_METHOD_CODE: // 405
         case SIP_TEMPORARILY_UNAVAILABLE_CODE: // 480
         case SIP_BAD_TRANSACTION_CODE: // 481
         case SIP_BAD_EVENT_CODE: // 489
         case SIP_UNIMPLEMENTED_METHOD_CODE: // 501
            if (seqMethod.compareTo(SIP_INVITE_METHOD) == 0)
            {
               trackTransactionResponse(sipMessage);
               SipResponseTransitionMemory memory(statusCode, statusText);
               return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
            }
            break;
         // these only affect transaction - if seqMethod was INVITE and media session doesn't exist, then destroy dialog
         // if media session exists, then no need to destroy the dialog - only INVITE failed for some reason, we can keep
         // the original media session
         default:
            // handle unknown 4XX, 5XX, 6XX
            CpSipTransactionManager::InviteTransactionState inviteState = getClientTransactionManager().getInviteTransactionState();
            if (statusCode >= SIP_4XX_CLASS_CODE && statusCode < SIP_7XX_CLASS_CODE &&
                statusCode != SIP_SMALL_SESSION_INTERVAL_CODE && // 422 means we can still retry
                seqMethod.compareTo(SIP_INVITE_METHOD) == 0 &&
                inviteState == CpSipTransactionManager::INITIAL_INVITE_ACTIVE)
            {
               // failure during initial INVITE, terminate dialog
               trackTransactionResponse(sipMessage);
               if (connectionState == ISipConnectionState::CONNECTION_REMOTE_OFFERING && m_rStateContext.m_bRedirecting)
               {
                  // we are redirecting and redirect failed, try next one
                  return followNextRedirect();
               }
               else
               {
                  SipResponseTransitionMemory memory(statusCode, statusText);
                  return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
               }
            }
            // otherwise re-INVITE failed
            ;
         }

         SipConnectionStateTransition* pTransition = processNonFatalResponse(sipMessage);
         trackTransactionResponse(sipMessage);
         return pTransition;
      }
      // if transaction was not found or is terminated then ignore response
   }

   trackTransactionResponse(sipMessage);
   return NULL; // no transition
}

SipConnectionStateTransition* BaseSipConnectionState::processNonFatalResponse(const SipMessage& sipMessage)
{
   // process inbound sip message request
   int seqNum;
   UtlString seqMethod;
   sipMessage.getCSeqField(seqNum, seqMethod);

   if (seqMethod.compareTo(SIP_INVITE_METHOD) == 0)
   {
      return processInviteResponse(sipMessage);
   }
   else if (seqMethod.compareTo(SIP_UPDATE_METHOD) == 0)
   {
      return processUpdateResponse(sipMessage);
   }
   else if (seqMethod.compareTo(SIP_BYE_METHOD) == 0)
   {
      return processByeResponse(sipMessage);
   }
   else if (seqMethod.compareTo(SIP_CANCEL_METHOD) == 0)
   {
      return processCancelResponse(sipMessage);
   }
   else if (seqMethod.compareTo(SIP_INFO_METHOD) == 0)
   {
      return processInfoResponse(sipMessage);
   }
   else if (seqMethod.compareTo(SIP_NOTIFY_METHOD) == 0)
   {
      return processNotifyResponse(sipMessage);
   }
   else if (seqMethod.compareTo(SIP_OPTIONS_METHOD) == 0)
   {
      return processOptionsResponse(sipMessage);
   }
   else if (seqMethod.compareTo(SIP_REFER_METHOD) == 0)
   {
      return processReferResponse(sipMessage);
   }
   else if (seqMethod.compareTo(SIP_SUBSCRIBE_METHOD) == 0)
   {
      // TODO: receiving in dialog subscribe is currently not enabled in XCpCallManager. Investigate
   }
   else if (seqMethod.compareTo(SIP_PRACK_METHOD) == 0)
   {
      return processPrackResponse(sipMessage);
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processInviteResponse(const SipMessage& sipMessage)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   // INVITE transaction termination is handled during disconnected state entry
   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED &&
      connectionState != ISipConnectionState::CONNECTION_UNKNOWN)
   {
      int responseCode = sipMessage.getResponseStatusCode();
      
      if (responseCode > SIP_1XX_CLASS_CODE &&
          responseCode < SIP_3XX_CLASS_CODE)
      {
         // update remote allow
         UtlString allowField;
         sipMessage.getAllowField(allowField);
         if (!allowField.isNull())
         {
            m_rStateContext.m_allowedRemote = allowField;
         }
         else
         {
            checkRemoteAllow(); // check if we know remote allow
         }
      }

      switch (responseCode)
      {
      case SIP_SMALL_SESSION_INTERVAL_CODE:
         handleSmallInviteSessionInterval(sipMessage);
         break;
      default:
         ;
      }
   }
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processUpdateResponse(const SipMessage& sipMessage)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   int responseCode = sipMessage.getResponseStatusCode();

   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED &&
       connectionState != ISipConnectionState::CONNECTION_UNKNOWN &&
       connectionState != ISipConnectionState::CONNECTION_IDLE &&
       connectionState != ISipConnectionState::CONNECTION_DIALING &&
       connectionState != ISipConnectionState::CONNECTION_NEWCALL)
   {
      if (responseCode >= SIP_2XX_CLASS_CODE && responseCode < SIP_3XX_CLASS_CODE)
      {
         // UPDATE transaction exists (checked in generic response handler), and we are in correct state
         if (m_rStateContext.m_sdpNegotiation.isInSdpNegotiation(sipMessage))
         {
            if (handleSdpAnswer(sipMessage))
            {
               commitMediaSessionChanges();
            }
         }
      } // if error then nothing happens
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processByeResponse(const SipMessage& sipMessage)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   // BYE is only valid if connection is established
   if (connectionState == ISipConnectionState::CONNECTION_ESTABLISHED)
   {
      int responseCode = sipMessage.getResponseStatusCode();
      UtlString responseText;
      sipMessage.getResponseStatusText(&responseText);

      if (responseCode == SIP_OK_CODE)
      {
         // progress to disconnected state
         SipResponseTransitionMemory memory(responseCode, responseText);
         return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
      }
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processCancelResponse(const SipMessage& sipMessage)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   // CANCEL is only valid if connection is not disconnected
   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED)
   {
      int responseCode = sipMessage.getResponseStatusCode();
      UtlString responseText;
      int sequenceNum;
      UtlString sequenceMethod;

      sipMessage.getResponseStatusText(&responseText);
      sipMessage.getCSeqField(&sequenceNum, &sequenceMethod);

      // only CANCEL for INVITE is supported, other methods provide immediate response
      CpSipTransactionManager::TransactionState transactionState = getClientTransactionManager().getTransactionState(SIP_INVITE_METHOD, sequenceNum);
      if (transactionState == CpSipTransactionManager::TRANSACTION_ACTIVE)
      {
         getClientTransactionManager().endTransaction(SIP_INVITE_METHOD, sequenceNum);
         if (responseCode == SIP_REQUEST_TERMINATED_CODE)
         {
            // progress to disconnected state
            SipResponseTransitionMemory memory(responseCode, responseText);
            return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
         }
      }
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processInfoResponse(const SipMessage& sipMessage)
{
   int responseCode = sipMessage.getResponseStatusCode();
   UtlString responseText;
   int cseqNum;
   UtlString cseqMethod;

   sipMessage.getResponseStatusText(&responseText);
   sipMessage.getCSeqField(&cseqNum, &cseqMethod);

   void* pCookie = getClientTransactionManager().getTransactionData(cseqMethod, cseqNum);
   // ignore 100 Trying
   if (responseCode >= SIP_2XX_CLASS_CODE && responseCode < SIP_3XX_CLASS_CODE)
   {
      m_rSipConnectionEventSink.fireSipXInfoStatusEvent(CP_INFOSTATUS_RESPONSE, SIPXTACK_MESSAGE_OK, responseText, responseCode, pCookie);
   }
   else if (responseCode >= SIP_3XX_CLASS_CODE && responseCode < SIP_5XX_CLASS_CODE)
   {
      m_rSipConnectionEventSink.fireSipXInfoStatusEvent(CP_INFOSTATUS_RESPONSE, SIPXTACK_MESSAGE_FAILURE, responseText, responseCode, pCookie);
   }
   else if (responseCode >= SIP_5XX_CLASS_CODE && responseCode < SIP_6XX_CLASS_CODE)
   {
      m_rSipConnectionEventSink.fireSipXInfoStatusEvent(CP_INFOSTATUS_RESPONSE, SIPXTACK_MESSAGE_SERVER_FAILURE, responseText, responseCode, pCookie);
   }
   else if (responseCode >= SIP_6XX_CLASS_CODE)
   {
      m_rSipConnectionEventSink.fireSipXInfoStatusEvent(CP_INFOSTATUS_RESPONSE, SIPXTACK_MESSAGE_GLOBAL_FAILURE, responseText, responseCode, pCookie);
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processOptionsResponse(const SipMessage& sipMessage)
{
   int responseCode = sipMessage.getResponseStatusCode();

   if (responseCode == SIP_OK_CODE)
   {
      UtlString allowField;
      sipMessage.getAllowField(allowField);
      if (!allowField.isNull())
      {
         m_rStateContext.m_allowedRemote = allowField;
      }
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processNotifyResponse(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processReferResponse(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processPrackResponse(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleAuthenticationRetryEvent(const SipMessage& sipMessage)
{

   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED &&
      connectionState != ISipConnectionState::CONNECTION_UNKNOWN)
   {
      if (sipMessage.isResponse())
      {
         int seqNum;
         UtlString seqMethod;
         sipMessage.getCSeqField(seqNum, seqMethod);

         if (needsTransactionTracking(seqMethod))
         {
            // start next transaction for authentication retry request we never see here
            if (seqMethod.compareTo(SIP_INVITE_METHOD) == 0)
            {
               getClientTransactionManager().updateInviteTransaction(seqNum + 1);
            }
            else
            {
               // non INVITE transaction
               getClientTransactionManager().endTransaction(seqNum); // end current transaction
               // SipUserAgent uses seqNum+1 for authentication retransmission. That works only
               // if we don't send 2 messages quickly that both need to be authenticated
               getNextLocalCSeq(); // skip 1 cseq number
               getClientTransactionManager().startTransaction(seqMethod, seqNum + 1); // start new transaction with the same method
            }

            // also update sdp negotiation for INVITE & UPDATE
            if (seqMethod.compareTo(SIP_INVITE_METHOD) == 0 ||
                seqMethod.compareTo(SIP_UPDATE_METHOD) == 0)
            {
               if (m_rStateContext.m_sdpNegotiation.isInSdpNegotiation(sipMessage))
               {
                  // original message was in SDP negotiation
                  m_rStateContext.m_sdpNegotiation.notifyAuthRetry(seqNum + 1);
               }
            }
         }
      }
   }

   // no state transition
   return NULL;
}

UtlBoolean BaseSipConnectionState::isMethodAllowed(const UtlString& sMethod)
{
   if (m_rStateContext.m_allowedRemote.index(sMethod) >=0 ||
       m_rStateContext.m_implicitAllowedRemote.index(sMethod) >= 0)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

void BaseSipConnectionState::deleteMediaConnection()
{
   CpMediaInterface* pInterface = m_rMediaInterfaceProvider.getMediaInterface(FALSE);

   if (pInterface)
   {
      int mediaConnectionId = getMediaConnectionId();

      if (mediaConnectionId != CpMediaInterface::INVALID_CONNECTION_ID)
      {
         setMediaConnectionId(CpMediaInterface::INVALID_CONNECTION_ID);

         // media interface exists, shut down media connection
         pInterface->stopRtpSend(mediaConnectionId);
         pInterface->stopRtpReceive(mediaConnectionId);
         pInterface->deleteConnection(mediaConnectionId);

         setRemoteMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_NONE, FALSE);
         setLocalMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_NONE);
      }
   }
}

void BaseSipConnectionState::terminateSipDialog()
{
   {
      OsWriteLock lock(m_rStateContext);
      m_rStateContext.m_sipDialog.terminateDialog();
   }
}

UtlBoolean BaseSipConnectionState::isLocalInitiatedDialog()
{
   OsWriteLock lock(m_rStateContext);
   return m_rStateContext.m_sipDialog.isLocalInitiatedDialog();
}

CpSipTransactionManager& BaseSipConnectionState::getClientTransactionManager() const
{
   return m_rStateContext.m_sipClientTransactionMgr;
}

CpSipTransactionManager& BaseSipConnectionState::getServerTransactionManager() const
{
   return m_rStateContext.m_sipServerTransactionMgr;
}

int BaseSipConnectionState::getMediaConnectionId() const
{
   // no need to lock atomic
   return m_rStateContext.m_mediaConnectionId;
}

void BaseSipConnectionState::setMediaConnectionId(int mediaConnectionId)
{
   // no need to lock atomic
   m_rStateContext.m_mediaConnectionId = mediaConnectionId;

   if (mediaConnectionId != CpMediaInterface::INVALID_CONNECTION_ID)
   {
      // this one is only used for routing media events after media connection destruction
      m_rStateContext.m_mediaEventConnectionId = mediaConnectionId;
   }
}

SipConnectionStateTransition* BaseSipConnectionState::handle100RelTimerMessage(const Sc100RelTimerMsg& timerMsg)
{
   // TODO: implement handler
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handle2xxTimerMessage(const Sc2xxTimerMsg& timerMsg)
{
   UtlBoolean bDeleteTimer = TRUE;

   if (!m_rStateContext.m_bAckReceived && m_rStateContext.m_pLastSent2xxToInvite)
   {
      if (m_rStateContext.m_i2xxInviteRetransmitCount < 8)
      {
         // resend message
         sendMessage(*m_rStateContext.m_pLastSent2xxToInvite);
         start2xxRetransmitTimer();
         bDeleteTimer = FALSE;
      }
   }

   if (bDeleteTimer)
   {
      delete2xxRetransmitTimer();
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleDisconnectTimerMessage(const ScDisconnectTimerMsg& timerMsg)
{
   return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, NULL);
}

SipConnectionStateTransition* BaseSipConnectionState::handleReInviteTimerMessage(const ScReInviteTimerMsg& timerMsg)
{
   switch (timerMsg.getReason())
   {
   case ScReInviteTimerMsg::REASON_NORMAL:
   case ScReInviteTimerMsg::REASON_HOLD:
   case ScReInviteTimerMsg::REASON_UNHOLD:
      return handleRenegotiateTimerMessage(timerMsg);
   case ScReInviteTimerMsg::REASON_SESSION_EXTENSION:
      // TODO: implement handler
      break;
   default:
      break;
   }
   
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleRenegotiateTimerMessage(const ScReInviteTimerMsg& timerMsg)
{
   deleteSessionRenegotiationTimer();

   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState == ISipConnectionState::CONNECTION_ESTABLISHED)
   {
      // we are in a state where we can do hold
      if (mayRenegotiateMediaSession())
      {
         switch (timerMsg.getReason())
         {
         case ScReInviteTimerMsg::REASON_NORMAL:
            renegotiateMediaSession();
            break;
         case ScReInviteTimerMsg::REASON_HOLD:
            doHold();
            break;
         case ScReInviteTimerMsg::REASON_UNHOLD:
            doUnhold();
            break;
         default:
            ;
         }
      }
      else
      {
         if (m_rStateContext.m_iRenegotiationRetryCount < MAX_RENEGOTIATION_RETRY_COUNT)
         {
            // we may try again
            startSessionRenegotiationTimer(timerMsg.getReason(), FALSE);
         }
         else
         {
            OsSysLog::add(FAC_CP, PRI_WARNING, "Giving up renegotiate (%i) operation, not ready to renegotiate session after %i tries\n",
               (int)timerMsg.getReason(), MAX_RENEGOTIATION_RETRY_COUNT);
         }
      }
   }

   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleByeRetryTimerMessage(const ScByeRetryTimerMsg& timerMsg)
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   deleteByeRetryTimer();

   // INVITE transaction termination is handled during disconnected state entry
   if (connectionState == ISipConnectionState::CONNECTION_ESTABLISHED)
   {
      if (m_rStateContext.m_bAckReceived)
      {
         // we may now send bye
         OsStatus result;
         return doByeConnection(result);
      }
      else
      {
         m_rStateContext.m_iByeRetryCount++;
         if (m_rStateContext.m_iByeRetryCount < 5)
         {
            // retry again
            startByeRetryTimer();
         }
         else
         {
            // too many tries
            return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, NULL);
         }
      }
   }

   return NULL;
}

UtlString BaseSipConnectionState::getCallId() const
{
   OsReadLock lock(m_rStateContext);
   return m_rStateContext.m_sipDialog.getCallId();
}

UtlBoolean BaseSipConnectionState::needsTransactionTracking(const UtlString& sipMethod) const
{
   UtlString trackable(TRACKABLE_METHODS);

   if (trackable.index(sipMethod) != UtlString::UTLSTRING_NOT_FOUND)
   {
      return TRUE;
   }

   return FALSE;
}

UtlString BaseSipConnectionState::buildContactUrl(const Url& fromUrl) const
{
   UtlString sContact;
   if (m_rStateContext.m_contactId != AUTOMATIC_CONTACT_ID)
   {
      // contact id is known
      SIPX_CONTACT_ADDRESS* pContact = m_rSipUserAgent.getContactDb().find(m_rStateContext.m_contactId);
      if (pContact != NULL)
      {
         // Get display name and user id from from Url
         UtlString displayName;
         UtlString userId;
         fromUrl.getDisplayName(displayName);
         fromUrl.getUserId(userId);

         Url contactUrl;
         contactUrl.setDisplayName(displayName);
         contactUrl.setUserId(userId);
         contactUrl.setHostAddress(pContact->cIpAddress);
         contactUrl.setHostPort(pContact->iPort);
         contactUrl.includeAngleBrackets();
         secureUrl(contactUrl);
         contactUrl.toString(sContact);
         return sContact;
      }
   }

   return buildDefaultContactUrl(fromUrl);
}

void BaseSipConnectionState::getLocalContactUrl(Url& contactUrl)
{
   Url localField;
   {
      OsReadLock lock(m_rStateContext);
      m_rStateContext.m_sipDialog.getLocalContact(contactUrl);
   }
}

UtlString BaseSipConnectionState::getLocalContactUrl()
{
   Url localContactUrl;
   getLocalContactUrl(localContactUrl);
   return localContactUrl.toString();
}

UtlString BaseSipConnectionState::buildDefaultContactUrl(const Url& fromUrl) const
{
   // automatic contact or id not found
   // Get host and port from default local contact
   UtlString address;
   UtlString contactHostPort;
   m_rSipUserAgent.getContactUri(&contactHostPort);
   Url hostPort(contactHostPort);
   hostPort.getHostAddress(address);
   int port = hostPort.getHostPort();

   // Get display name and user id from from Url
   UtlString displayName;
   UtlString userId;
   fromUrl.getDisplayName(displayName);
   fromUrl.getUserId(userId);

   // Construct a new contact URL with host/port from local contact
   // and display name/userid from From URL
   Url contactUrl;
   contactUrl.setDisplayName(displayName);
   contactUrl.setUserId(userId);
   contactUrl.setHostAddress(address);
   contactUrl.setHostPort(port);
   contactUrl.includeAngleBrackets();
   return contactUrl.toString();
}

void BaseSipConnectionState::secureUrl(Url& fromUrl) const
{
   SIPX_CONTACT_ADDRESS* pContact = m_rSipUserAgent.getContactDb().find(m_rStateContext.m_contactId);
   if (pContact != NULL)
   {
      if (pContact->eTransportType == TRANSPORT_TLS)
      {
         fromUrl.setScheme(Url::SipsUrlScheme);
      }
   }
}

UtlBoolean BaseSipConnectionState::setupMediaConnection(RTP_TRANSPORT rtpTransportOptions, int& mediaConnectionId)
{
   OsStatus res = m_rMediaInterfaceProvider.getMediaInterface()->createConnection(mediaConnectionId,
               NULL,
               0,
               NULL, // no display settings
               (void*)m_rStateContext.m_pSecurity,
               &m_rMessageQueueProvider.getLocalQueue(),
               rtpTransportOptions);
   
   if (res == OS_SUCCESS)
   {
      setMediaConnectionId(mediaConnectionId);
      return TRUE;
   }

   return FALSE;
}

UtlBoolean BaseSipConnectionState::prepareSdpOffer(SipMessage& sipMessage)
{
   m_rStateContext.m_sdpNegotiation.startSdpNegotiation(sipMessage, m_rStateContext.m_bUseLocalHoldSDP);
   CpMediaInterface* pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface();

   int mediaConnectionId = getMediaConnectionId();
   if (mediaConnectionId == CpMediaInterface::INVALID_CONNECTION_ID)
   {
      if (!setupMediaConnection(m_rStateContext.m_rtpTransport, mediaConnectionId))
      {
         return FALSE;
      }
   }

   SIPX_CONTACT_ADDRESS* pContact = m_rSipUserAgent.getContactDb().find(m_rStateContext.m_contactId);
   if (pContact != NULL)
   {
      pMediaInterface->setContactType(mediaConnectionId, pContact->eContactType, m_rStateContext.m_contactId);
   }
   else
   {
      pMediaInterface->setContactType(mediaConnectionId, (SIPX_CONTACT_TYPE)AUTOMATIC_CONTACT_TYPE, AUTOMATIC_CONTACT_ID);
   }

   UtlString hostAddresses[MAX_ADDRESS_CANDIDATES];
   int receiveRtpPorts[MAX_ADDRESS_CANDIDATES];
   int receiveRtcpPorts[MAX_ADDRESS_CANDIDATES];
   int receiveVideoRtpPorts[MAX_ADDRESS_CANDIDATES];
   int receiveVideoRtcpPorts[MAX_ADDRESS_CANDIDATES];
   RTP_TRANSPORT transportTypes[MAX_ADDRESS_CANDIDATES];
   int nRtpContacts;
   int totalBandwidth = 0;
   SdpSrtpParameters srtpParams;
   SdpCodecList supportedCodecs;
   int videoFramerate = 0;
   const int bandWidth = AUDIO_MICODEC_BW_DEFAULT;
   OsStatus res = pMediaInterface->getCapabilitiesEx(mediaConnectionId,
         MAX_ADDRESS_CANDIDATES,
         hostAddresses,
         receiveRtpPorts,
         receiveRtcpPorts,
         receiveVideoRtpPorts,
         receiveVideoRtcpPorts,
         transportTypes,
         nRtpContacts,
         supportedCodecs,
         srtpParams,
         bandWidth,
         totalBandwidth,
         videoFramerate);

   if (res == OS_SUCCESS)
   {
      if (!m_natTraversalConfig.m_bEnableICE)
      {
         nRtpContacts = 1;
      }

      m_rStateContext.m_sdpNegotiation.addSdpOffer(sipMessage,
         nRtpContacts, hostAddresses, receiveRtpPorts, receiveRtcpPorts, receiveVideoRtpPorts, receiveVideoRtcpPorts,
         transportTypes, supportedCodecs, srtpParams, totalBandwidth, videoFramerate, m_rStateContext.m_rtpTransport);
      return TRUE;
   }

   // creation of SDP offer failed for some reason, reset SDP negotiation
   m_rStateContext.m_sdpNegotiation.resetSdpNegotiation();
   return FALSE;
}

UtlBoolean BaseSipConnectionState::handleSdpOffer(const SipMessage& sipMessage)
{
   const SdpBody* pSdpBody = sipMessage.getSdpBody(m_rStateContext.m_pSecurity);
   if (pSdpBody)
   {
      m_rStateContext.m_sdpNegotiation.startSdpNegotiation(sipMessage, m_rStateContext.m_bUseLocalHoldSDP);
      CpMediaInterface* pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface();
      int mediaConnectionId = getMediaConnectionId();

      if (mediaConnectionId == CpMediaInterface::INVALID_CONNECTION_ID)
      {
         if (!setupMediaConnection(m_rStateContext.m_rtpTransport, mediaConnectionId))
         {
            // handling of SDP offer failed for some reason, reset SDP negotiation
            m_rStateContext.m_sdpNegotiation.resetSdpNegotiation();
            return FALSE;
         }
      }

      m_rStateContext.m_sdpNegotiation.handleInboundSdpOffer(sipMessage);
      return handleRemoteSdpBody(*pSdpBody);
   }

   // handling of SDP offer failed for some reason, reset SDP negotiation
   m_rStateContext.m_sdpNegotiation.resetSdpNegotiation();
   return FALSE;
}

UtlBoolean BaseSipConnectionState::prepareSdpAnswer(SipMessage& sipMessage)
{
   CpMediaInterface* pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface();

   int mediaConnectionId = getMediaConnectionId();
   if (mediaConnectionId == CpMediaInterface::INVALID_CONNECTION_ID)
   {
      return FALSE;
   }

   SIPX_CONTACT_ADDRESS* pContact = m_rSipUserAgent.getContactDb().find(m_rStateContext.m_contactId);
   if (pContact != NULL)
   {
      pMediaInterface->setContactType(mediaConnectionId, pContact->eContactType, m_rStateContext.m_contactId);
   }
   else
   {
      pMediaInterface->setContactType(mediaConnectionId, (SIPX_CONTACT_TYPE)AUTOMATIC_CONTACT_TYPE, AUTOMATIC_CONTACT_ID);
   }

   UtlString hostAddresses[MAX_ADDRESS_CANDIDATES];
   int receiveRtpPorts[MAX_ADDRESS_CANDIDATES];
   int receiveRtcpPorts[MAX_ADDRESS_CANDIDATES];
   int receiveVideoRtpPorts[MAX_ADDRESS_CANDIDATES];
   int receiveVideoRtcpPorts[MAX_ADDRESS_CANDIDATES];
   RTP_TRANSPORT transportTypes[MAX_ADDRESS_CANDIDATES];
   int nRtpContacts;
   int totalBandwidth = 0;
   SdpSrtpParameters srtpParams;
   SdpCodecList supportedCodecs;
   int videoFramerate = 0;
   const int bandWidth = AUDIO_MICODEC_BW_DEFAULT;
   OsStatus res = pMediaInterface->getCapabilitiesEx(mediaConnectionId,
      MAX_ADDRESS_CANDIDATES,
      hostAddresses,
      receiveRtpPorts,
      receiveRtcpPorts,
      receiveVideoRtpPorts,
      receiveVideoRtcpPorts,
      transportTypes,
      nRtpContacts,
      supportedCodecs,
      srtpParams,
      bandWidth,
      totalBandwidth,
      videoFramerate);

   if (res == OS_SUCCESS)
   {
      if (!m_natTraversalConfig.m_bEnableICE)
      {
         nRtpContacts = 1;
      }

      m_rStateContext.m_sdpNegotiation.addSdpAnswer(sipMessage,
         nRtpContacts, hostAddresses, receiveRtpPorts, receiveRtcpPorts, receiveVideoRtpPorts, receiveVideoRtcpPorts,
         transportTypes, supportedCodecs, srtpParams, totalBandwidth, videoFramerate, m_rStateContext.m_rtpTransport);
      return TRUE;
   }

   // creation of SDP answer failed for some reason, reset SDP negotiation
   m_rStateContext.m_sdpNegotiation.resetSdpNegotiation();

   return FALSE;
}

UtlBoolean BaseSipConnectionState::handleSdpAnswer(const SipMessage& sipMessage)
{
   const SdpBody* pSdpBody = sipMessage.getSdpBody(m_rStateContext.m_pSecurity);

   if (pSdpBody)
   {
      m_rStateContext.m_sdpNegotiation.handleInboundSdpAnswer(sipMessage);
      return handleRemoteSdpBody(*pSdpBody);
   }
   else
   {
      // no SDP in answer, reset SDP negotiation
      m_rStateContext.m_sdpNegotiation.resetSdpNegotiation();
   }

   return FALSE;
}

UtlBoolean BaseSipConnectionState::handleRemoteSdpBody(const SdpBody& sdpBody)
{
   UtlString rtpAddress;
   int totalBandwidth = 0;
   int matchingBandwidth = 0;
   int videoFramerate = 0;
   int matchingVideoFramerate = 0;
   SdpCodecList supportedCodecs;
   SdpCodecList commonCodecsForEncoder;
   SdpCodecList commonCodecsForDecoder;
   SdpSrtpParameters srtpParams;
   memset(&srtpParams, 0, sizeof(srtpParams));
   int numMatchingCodecs = 0;
   SdpSrtpParameters matchingSrtpParams;
   memset(&matchingSrtpParams, 0, sizeof(matchingSrtpParams));
   UtlString remoteRtpAddress; // only used if ICE is disabled
   int remoteRtpPort = -1; // only used if ICE is disabled
   int remoteRtcpPort = -1; // only used if ICE is disabled
   int remoteVideoRtpPort = -1; // only used if ICE is disabled
   int remoteVideoRtcpPort = -1; // only used if ICE is disabled

   int mediaConnectionId = getMediaConnectionId();
   CpMediaInterface* pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface();
   pMediaInterface->getCodecList(mediaConnectionId, supportedCodecs);

   CpSdpNegotiation::getCommonSdpCodecs(sdpBody,
      supportedCodecs, numMatchingCodecs,
      commonCodecsForEncoder, commonCodecsForDecoder,
      remoteRtpAddress, remoteRtpPort, remoteRtcpPort, remoteVideoRtpPort, remoteVideoRtcpPort,
      srtpParams, matchingSrtpParams,
      totalBandwidth, matchingBandwidth,
      videoFramerate, matchingVideoFramerate);

   if (numMatchingCodecs > 0)
   {
      return TRUE;
   }

   // handling of SDP body failed for some reason, reset SDP negotiation
   m_rStateContext.m_sdpNegotiation.resetSdpNegotiation();
   return FALSE;
}

UtlBoolean BaseSipConnectionState::commitMediaSessionChanges()
{
   if (m_rStateContext.m_sdpNegotiation.getNegotiationState() == CpSdpNegotiation::SDP_NEGOTIATION_COMPLETE)
   {
      SdpCodecList localSdpCodecList;
      m_rStateContext.m_sdpNegotiation.getLocalSdpCodecList(localSdpCodecList);
      SdpBody sdpBody;
      UtlBoolean bodyFound = m_rStateContext.m_sdpNegotiation.getRemoteSdpBody(sdpBody);

      if (bodyFound)
      {
         UtlString rtpAddress;
         int totalBandwidth = 0;
         int matchingBandwidth = 0;
         int videoFramerate = 0;
         int matchingVideoFramerate = 0;
         SdpCodecList supportedCodecs;
         SdpCodecList commonCodecsForEncoder;
         SdpCodecList commonCodecsForDecoder;
         SdpSrtpParameters srtpParams;
         memset(&srtpParams, 0, sizeof(srtpParams));
         int numMatchingCodecs = 0;
         SdpSrtpParameters matchingSrtpParams;
         memset(&matchingSrtpParams, 0, sizeof(matchingSrtpParams));
         UtlString remoteRtpAddress; // only used if ICE is disabled
         int remoteRtpPort = -1; // only used if ICE is disabled
         int remoteRtcpPort = -1; // only used if ICE is disabled
         int remoteVideoRtpPort = -1; // only used if ICE is disabled
         int remoteVideoRtcpPort = -1; // only used if ICE is disabled

         int mediaConnectionId = getMediaConnectionId();
         CpMediaInterface* pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface();
         pMediaInterface->getCodecList(mediaConnectionId, supportedCodecs);

         CpSdpNegotiation::getCommonSdpCodecs(sdpBody,
            supportedCodecs, numMatchingCodecs,
            commonCodecsForEncoder, commonCodecsForDecoder,
            remoteRtpAddress, remoteRtpPort, remoteRtcpPort, remoteVideoRtpPort, remoteVideoRtcpPort,
            srtpParams, matchingSrtpParams,
            totalBandwidth, matchingBandwidth,
            videoFramerate, matchingVideoFramerate);

         if (numMatchingCodecs > 0)
         {
            if (matchingBandwidth != 0)
            {
               pMediaInterface->setConnectionBitrate(mediaConnectionId, matchingBandwidth);
            }
            if (matchingVideoFramerate != 0)
            {
               pMediaInterface->setConnectionFramerate(mediaConnectionId, matchingVideoFramerate);
            }
            // Set up the remote RTP sockets
            setMediaDestination(remoteRtpAddress.data(),
               remoteRtpPort,
               remoteRtcpPort,
               remoteVideoRtpPort,
               remoteVideoRtcpPort,
               &sdpBody);

            if (remoteRtpAddress.compareTo("0.0.0.0") == 0) // hold address
            {
               pMediaInterface->stopRtpSend(mediaConnectionId);
               setRemoteMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_HELD, FALSE);
            }
            else
            {
               pMediaInterface->startRtpSend(mediaConnectionId, commonCodecsForEncoder);
               setRemoteMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_ACTIVE, FALSE);
            }

            if (!m_rStateContext.m_bUseLocalHoldSDP)
            {
               pMediaInterface->startRtpReceive(mediaConnectionId, localSdpCodecList);
               setLocalMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_ACTIVE);
            }
            else
            {
               // local hold was negotiated
               pMediaInterface->stopRtpReceive(mediaConnectionId);
               setLocalMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_HELD);
            }

            return TRUE;
         }
         else
         {
            setRemoteMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_NONE, FALSE);
            setLocalMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_NONE);
         }
      }
   }

   return FALSE;
}

void BaseSipConnectionState::setLastSentInvite(const SipMessage& sipMessage)
{
   if (m_rStateContext.m_pLastSentInvite)
   {
      delete m_rStateContext.m_pLastSentInvite;
      m_rStateContext.m_pLastSentInvite = NULL;
   }

   m_rStateContext.m_pLastSentInvite = new SipMessage(sipMessage);
}

void BaseSipConnectionState::setLastReceivedInvite(const SipMessage& sipMessage)
{
   if (m_rStateContext.m_pLastReceivedInvite)
   {
      delete m_rStateContext.m_pLastReceivedInvite;
      m_rStateContext.m_pLastReceivedInvite = NULL;
   }

   m_rStateContext.m_pLastReceivedInvite = new SipMessage(sipMessage);
}

void BaseSipConnectionState::setLastSent2xxToInvite(const SipMessage& sipMessage)
{
   if (m_rStateContext.m_pLastSent2xxToInvite)
   {
      delete m_rStateContext.m_pLastSent2xxToInvite;
      m_rStateContext.m_pLastSent2xxToInvite = NULL;
   }

   m_rStateContext.m_pLastSent2xxToInvite = new SipMessage(sipMessage);
}

CpSessionTimerProperties& BaseSipConnectionState::getSessionTimerProperties()
{
   return m_rStateContext.m_sessionTimerProperties;
}

void BaseSipConnectionState::handleSmallInviteSessionInterval(const SipMessage& sipMessage)
{
   int minSe = 90;
   int iSeqNumber = 0;
   UtlString seqMethod;

   sipMessage.getCSeqField(iSeqNumber, seqMethod);
   CpSipTransactionManager::InviteTransactionState inviteState = getClientTransactionManager().getInviteTransactionState();
   getClientTransactionManager().endTransaction(seqMethod, iSeqNumber);

   if (sipMessage.getMinSe(minSe) && inviteState != CpSipTransactionManager::INVITE_INACTIVE)
   {
      m_rStateContext.m_sessionTimerProperties.setMinSessionExpires(minSe);
      m_rStateContext.m_sessionTimerProperties.setSessionExpires(minSe);

      if (m_rStateContext.m_pLastSentInvite && !getClientTransactionManager().isInviteTransactionActive())
      {
         SipMessage sipInvite(*m_rStateContext.m_pLastSentInvite);
         // update session expires
         int iSessionExpires;
         UtlString refresher;
         sipInvite.getSessionExpires(&iSessionExpires, &refresher);
         sipInvite.setSessionExpires(minSe, refresher); // set new session expires
         sipInvite.setMinSe(minSe);
         // update transaction
         int cseqNum = getNextLocalCSeq();
         sipInvite.setCSeqField(cseqNum, SIP_INVITE_METHOD);
         sendMessage(sipInvite); // also saves last invite message again
      }
   }
}

void BaseSipConnectionState::sendOptionsRequest()
{
   // allow was not set in response, send OPTIONS
   int iCSeq = getNextLocalCSeq();
   SipMessage optionsRequest;
   prepareSipRequest(optionsRequest, SIP_OPTIONS_METHOD, iCSeq);
   sendMessage(optionsRequest);
}

void BaseSipConnectionState::checkRemoteAllow()
{
   UtlBoolean bIsDialogEstablished = FALSE;
   {
      OsReadLock lock(m_rStateContext);
      bIsDialogEstablished = m_rStateContext.m_sipDialog.isEstablishedDialog();
   }

   if (m_rStateContext.m_allowedRemote.isNull() && bIsDialogEstablished)
   {
      // allow was not set in response, send in dialog OPTIONS
      sendOptionsRequest();
   }
}

void BaseSipConnectionState::handleInvite2xxResponse(const SipMessage& sipResponse)
{
   int responseCode = sipResponse.getResponseStatusCode();
   int seqNum;
   UtlString seqMethod;
   sipResponse.getCSeqField(&seqNum, &seqMethod);
   UtlBoolean bProtocolError = FALSE;

   // get pointer to internal sdp bodyContent
   const SdpBody* pSdpBody = sipResponse.getSdpBody(m_rStateContext.m_pSecurity);
   SipMessage sipRequest;
   prepareSipRequest(sipRequest, SIP_ACK_METHOD, seqNum);

   CpSdpNegotiation::SdpNegotiationState negotiationState = m_rStateContext.m_sdpNegotiation.getNegotiationState();

   if (pSdpBody)
   {
      // response has SDP bodyContent, we must provide SDP answer
      if (negotiationState == CpSdpNegotiation::SDP_NEGOTIATION_COMPLETE)
      {
         CpSdpNegotiation::SdpBodyType sdpBodyType = m_rStateContext.m_sdpNegotiation.getSdpBodyType(sipResponse);
         // this 200 OK is probably a retransmission
         if (sdpBodyType == CpSdpNegotiation::SDP_BODY_OFFER)
         {
            prepareSdpAnswer(sipRequest); // add sdp answer
         }
         else if (sdpBodyType == CpSdpNegotiation::SDP_BODY_UNKNOWN)
         {
            prepareSdpAnswer(sipRequest);
            bProtocolError = TRUE; // this is protocol error
         }
      }
      else if (negotiationState == CpSdpNegotiation::SDP_NEGOTIATION_IN_PROGRESS)
      {
         // message has sdp bodyContent, and we are in negotiation (SDP was in INVITE)
         handleSdpAnswer(sipResponse);
      }
      else if (negotiationState == CpSdpNegotiation::SDP_NOT_NEGOTIATED)
      {
         // delayed SDP negotiation
         bProtocolError = !handleSdpOffer(sipResponse); // if we get false, we need to terminate call
         if (!bProtocolError)
         {
            prepareSdpAnswer(sipRequest);
         } // if protocol error, then sdp negotiation was reset already, and we cannot send SDP answer in ACK
      }
      //prepareSdpAnswer
   }

   negotiationState = m_rStateContext.m_sdpNegotiation.getNegotiationState();
   if (negotiationState != CpSdpNegotiation::SDP_NEGOTIATION_COMPLETE)
   {
      bProtocolError = TRUE;
   }
   else
   {
      // SDP negotiation is complete, commit changes
      commitMediaSessionChanges();
   }

   sendMessage(sipRequest); // send ACK

   if (bProtocolError || m_rStateContext.m_bCallDisconnecting)
   {
      sendBye(); // also send BYE
   }
}

void BaseSipConnectionState::sendBye()
{
   if (!m_rStateContext.m_bByeSent)
   {
      // send BYE
      m_rStateContext.m_bByeSent = TRUE;
      SipMessage byeRequest;
      int seqNum = getNextLocalCSeq();
      prepareSipRequest(byeRequest, SIP_BYE_METHOD, seqNum);
      sendMessage(byeRequest);
      startByeTimeoutTimer(); // start bye timer to force destroy connection after some timeout
   }
}

void BaseSipConnectionState::sendInviteCancel()
{
   // send CANCEL
   if (m_rStateContext.m_pLastSentInvite)
   {
      int seqNum;
      UtlString seqMethod;
      m_rStateContext.m_pLastSentInvite->getCSeqField(seqNum, seqMethod);

      SipMessage cancelRequest;
      prepareSipRequest(cancelRequest, SIP_CANCEL_METHOD, seqNum);
      sendMessage(cancelRequest);
      startCancelTimeoutTimer(); // start cancel timer to force destroy connection after some timeout
   }
}

void BaseSipConnectionState::sendReInvite()
{
   SipMessage sipInvite;
   int seqNum = getNextLocalCSeq();

   sipInvite.setSecurityAttributes(m_rStateContext.m_pSecurity);
   prepareSipRequest(sipInvite, SIP_INVITE_METHOD, seqNum);

   int sessionExpires = m_rStateContext.m_sessionTimerProperties.getSessionExpires();
   sipInvite.setSessionExpires(sessionExpires);

   if (!m_rStateContext.m_locationHeader.isNull())
   {
      sipInvite.setLocationField(m_rStateContext.m_locationHeader);
   }

   // add SDP if negotiation mode is immediate, otherwise don't add it
   if (m_rStateContext.m_sdpNegotiation.getSdpOfferingMode() == CpSdpNegotiation::SDP_OFFERING_IMMEDIATE)
   {
      if (!prepareSdpOffer(sipInvite))
      {
         // SDP negotiation start failed
         OsSysLog::add(FAC_CP, PRI_ERR, "SDP preparation for re-INVITE failed.\n");
         return;
      }
   }

   // try to send sip message
   UtlBoolean sendSuccess = sendMessage(sipInvite);

   if (!sendSuccess)
   {
      OsSysLog::add(FAC_CP, PRI_ERR, "Sending re-INVITE failed.\n");
   }
}

void BaseSipConnectionState::sendUpdate()
{
   SipMessage sipUpdate;
   int seqNum = getNextLocalCSeq();

   sipUpdate.setSecurityAttributes(m_rStateContext.m_pSecurity);
   prepareSipRequest(sipUpdate, SIP_UPDATE_METHOD, seqNum);

   int sessionExpires = m_rStateContext.m_sessionTimerProperties.getSessionExpires();
   sipUpdate.setSessionExpires(sessionExpires);

   if (!m_rStateContext.m_locationHeader.isNull())
   {
      sipUpdate.setLocationField(m_rStateContext.m_locationHeader);
   }

   if (!prepareSdpOffer(sipUpdate))
   {
      // SDP negotiation start failed
      OsSysLog::add(FAC_CP, PRI_ERR, "SDP preparation for UPDATE failed.\n");
      return;
   }

   // try to send sip message
   UtlBoolean sendSuccess = sendMessage(sipUpdate);

   if (!sendSuccess)
   {
      OsSysLog::add(FAC_CP, PRI_ERR, "Sending UPDATE failed.\n");
   }
}

void BaseSipConnectionState::setMediaDestination(const char* hostAddress,
                                                 int audioRtpPort,
                                                 int audioRtcpPort,
                                                 int videoRtpPort,
                                                 int videoRtcpPort,
                                                 const SdpBody* pRemoteBody)
{
   UtlBoolean bSetDestination = FALSE;

   int mediaConnectionId = getMediaConnectionId();
   CpMediaInterface* pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface();

   /*
   * Assumption: that ICE is either enabled for both audio and video or not
   * at all.  If you attempt to mix, this won't work correctly.  To fix
   * this, we need to break setConnectionDestination(...) into two methods
   * -- one for audio and one for video.
   */
   if (pMediaInterface && hostAddress && (strcasecmp(hostAddress, "0.0.0.0") != 0))
   {
      if (pRemoteBody && m_natTraversalConfig.m_bEnableICE)
      {
         int         candidateIds[MAX_ADDRESS_CANDIDATES];
         UtlString   transportIds[MAX_ADDRESS_CANDIDATES];
         UtlString   transportTypes[MAX_ADDRESS_CANDIDATES];
         double      qValues[MAX_ADDRESS_CANDIDATES];
         UtlString   candidateIps[MAX_ADDRESS_CANDIDATES];
         int         candidatePorts[MAX_ADDRESS_CANDIDATES];
         int         nCandidates = 0;

         // Check for / add audio candidate addresses
         if (pRemoteBody->getCandidateAttributes(SDP_AUDIO_MEDIA_TYPE,
            MAX_ADDRESS_CANDIDATES,
            candidateIds,
            transportIds,
            transportTypes,
            qValues,
            candidateIps,
            candidatePorts,
            nCandidates))
         {
            bSetDestination = TRUE;

            int lastId = -1;
            for (int i=0; i<nCandidates; i++)
            {
               if (transportTypes[i].compareTo("UDP") == 0)
               {
                  if (candidateIds[i] != lastId)
                  {
                     if (pMediaInterface->addAudioRtpConnectionDestination(
                        mediaConnectionId,
                        (int) (qValues[i] * 100),
                        candidateIps[i],
                        candidatePorts[i]) != OS_SUCCESS)
                     {
                        OsSysLog::add(FAC_NET, PRI_ERR,
                           "Failed to set audio rtp media destination (%d %s %s:%d)",
                           candidateIds[i], transportIds[i].data(),
                           candidateIps[i].data(), candidatePorts[i]);

                     }
                  }
                  else
                  {
                     if (pMediaInterface->addAudioRtcpConnectionDestination(
                        mediaConnectionId,
                        (int) (qValues[i] * 100),
                        candidateIps[i],
                        candidatePorts[i]) != OS_SUCCESS)
                     {
                        OsSysLog::add(FAC_NET, PRI_ERR,
                           "Failed to set audio rtcp media destination (%d %s %s:%d)",
                           candidateIds[i], transportIds[i].data(),
                           candidateIps[i].data(), candidatePorts[i]);
                     }
                  }
                  lastId = candidateIds[i];
               }
            }
         }

         // Check for / add video candidate addresses
         if (pRemoteBody->getCandidateAttributes(SDP_VIDEO_MEDIA_TYPE,
            MAX_ADDRESS_CANDIDATES,
            candidateIds,
            transportIds,
            transportTypes,
            qValues,
            candidateIps,
            candidatePorts,
            nCandidates))
         {
            bSetDestination = TRUE;

            int lastId = -1;
            for (int i=0; i<nCandidates; i++)
            {
               if (transportTypes[i].compareTo("UDP") == 0)
               {
                  if (candidateIds[i] != lastId)
                  {
                     if (pMediaInterface->addVideoRtpConnectionDestination(
                        mediaConnectionId,
                        (int) (qValues[i] * 100),
                        candidateIps[i],
                        candidatePorts[i]) != OS_SUCCESS)
                     {
                        OsSysLog::add(FAC_NET, PRI_ERR,
                           "Failed to set video rtp media destination (%d %s %s:%d)",
                           candidateIds[i], transportIds[i].data(),
                           candidateIps[i].data(), candidatePorts[i]);

                     }
                  }
                  else
                  {
                     if (pMediaInterface->addVideoRtcpConnectionDestination(
                        mediaConnectionId,
                        (int) (qValues[i] * 100),
                        candidateIps[i],
                        candidatePorts[i]) != OS_SUCCESS)
                     {
                        OsSysLog::add(FAC_NET, PRI_ERR,
                           "Failed to set video rtcp media destination (%d %s %s:%d)",
                           candidateIds[i], transportIds[i].data(),
                           candidateIps[i].data(), candidatePorts[i]);
                     }
                  }
                  lastId = candidateIds[i];
               }
            }
         }
      }

      if (!bSetDestination)
      {
         pMediaInterface->setConnectionDestination(mediaConnectionId,
            hostAddress,
            audioRtpPort,
            audioRtcpPort,
            videoRtpPort,
            videoRtcpPort);
      }
   }
}

SipConnectionStateTransition* BaseSipConnectionState::doByeConnection(OsStatus& result)
{
   // if we are inbound call, sent 200 OK but haven't received ACK yet, we may not do BYE
   if (!m_rStateContext.m_bCallDisconnecting)
   {
      sendBye();
   }
   result = OS_SUCCESS;
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::doCancelConnection(OsStatus& result)
{
   if (!m_rStateContext.m_bCallDisconnecting)
   {
      sendInviteCancel();
   }
   result = OS_SUCCESS;
   return NULL;
}

void BaseSipConnectionState::requestConnectionDestruction()
{
   // send message to call that we want to destroy the XSipConnection
   SipDialog sipDialog;
   {
      OsReadLock lock(m_rStateContext);
      sipDialog = m_rStateContext.m_sipDialog;
   }
   AcDestroyConnectionMsg msg(sipDialog);
   m_rMessageQueueProvider.getLocalQueue().send(msg);
}

void BaseSipConnectionState::startCancelTimeoutTimer()
{
   deleteCancelTimeoutTimer();

   UtlString sipCallId;
   UtlString localTag;
   UtlString remoteTag;
   UtlBoolean isFromLocal;

   getSipDialogId(sipCallId, localTag, remoteTag, isFromLocal);
   ScDisconnectTimerMsg msg(ScDisconnectTimerMsg::REASON_CANCEL_TIMEOUT, sipCallId, localTag, remoteTag, isFromLocal);
   OsTimerNotification* pNotification = new OsTimerNotification(m_rMessageQueueProvider.getLocalQueue(), msg);
   m_rStateContext.m_pCancelTimeoutTimer = new OsTimer(pNotification);
   int sipTransactionTimeout = m_rSipUserAgent.getSipStateTransactionTimeout();
   OsTime timerTime(T1_PERIOD_MSEC * 64);
   m_rStateContext.m_pCancelTimeoutTimer->oneshotAfter(timerTime); // start timer
}

void BaseSipConnectionState::deleteCancelTimeoutTimer()
{
   if (m_rStateContext.m_pCancelTimeoutTimer)
   {
      delete m_rStateContext.m_pCancelTimeoutTimer;
      m_rStateContext.m_pCancelTimeoutTimer = NULL;
   }
}

void BaseSipConnectionState::startByeTimeoutTimer()
{
   deleteByeTimeoutTimer();

   UtlString sipCallId;
   UtlString localTag;
   UtlString remoteTag;
   UtlBoolean isFromLocal;

   getSipDialogId(sipCallId, localTag, remoteTag, isFromLocal);
   ScDisconnectTimerMsg msg(ScDisconnectTimerMsg::REASON_BYE_TIMEOUT, sipCallId, localTag, remoteTag, isFromLocal);
   OsTimerNotification* pNotification = new OsTimerNotification(m_rMessageQueueProvider.getLocalQueue(), msg);
   m_rStateContext.m_pByeTimeoutTimer = new OsTimer(pNotification);
   int sipTransactionTimeoutMs = m_rSipUserAgent.getSipStateTransactionTimeout();
   OsTime timerTime(sipTransactionTimeoutMs);
   m_rStateContext.m_pByeTimeoutTimer->oneshotAfter(timerTime); // start timer
}

void BaseSipConnectionState::deleteByeTimeoutTimer()
{
   if (m_rStateContext.m_pByeTimeoutTimer)
   {
      delete m_rStateContext.m_pByeTimeoutTimer;
      m_rStateContext.m_pByeTimeoutTimer = NULL;
   }
}

void BaseSipConnectionState::startByeRetryTimer()
{
   deleteByeRetryTimer();

   UtlString sipCallId;
   UtlString localTag;
   UtlString remoteTag;
   UtlBoolean isFromLocal;

   getSipDialogId(sipCallId, localTag, remoteTag, isFromLocal);
   ScByeRetryTimerMsg msg(sipCallId, localTag, remoteTag, isFromLocal);
   OsTimerNotification* pNotification = new OsTimerNotification(m_rMessageQueueProvider.getLocalQueue(), msg);
   m_rStateContext.m_pByeRetryTimer = new OsTimer(pNotification);
   OsTime timerTime(1, 0); // try BYE in 1 second again
   m_rStateContext.m_pByeRetryTimer->oneshotAfter(timerTime); // start timer
}

void BaseSipConnectionState::deleteByeRetryTimer()
{
   if (m_rStateContext.m_pByeRetryTimer)
   {
      delete m_rStateContext.m_pByeRetryTimer;
      m_rStateContext.m_pByeRetryTimer = NULL;
   }
}

void BaseSipConnectionState::start2xxRetransmitTimer()
{
   delete2xxRetransmitTimer();

   UtlString sipCallId;
   UtlString localTag;
   UtlString remoteTag;
   UtlBoolean isFromLocal;

   getSipDialogId(sipCallId, localTag, remoteTag, isFromLocal);
   Sc2xxTimerMsg msg(sipCallId, localTag, remoteTag, isFromLocal);
   OsTimerNotification* pNotification = new OsTimerNotification(m_rMessageQueueProvider.getLocalQueue(), msg);
   m_rStateContext.m_p2xxInviteRetransmitTimer = new OsTimer(pNotification);
   OsTime timerTime(max(T1_PERIOD_MSEC * (m_rStateContext.m_i2xxInviteRetransmitCount + 1), T2_PERIOD_MSEC));
   m_rStateContext.m_p2xxInviteRetransmitTimer->oneshotAfter(timerTime); // start timer
}

void BaseSipConnectionState::delete2xxRetransmitTimer()
{
   if (m_rStateContext.m_p2xxInviteRetransmitTimer)
   {
      delete m_rStateContext.m_p2xxInviteRetransmitTimer;
      m_rStateContext.m_p2xxInviteRetransmitTimer = NULL;
   }
}

SipConnectionStateTransition* BaseSipConnectionState::doRejectInboundConnectionInProgress(OsStatus& result)
{
   if (!m_rStateContext.m_bCallDisconnecting)
   {
      if (m_rStateContext.m_pLastReceivedInvite)
      {
         SipMessage sipResponse;
         sipResponse.setInviteForbidden(m_rStateContext.m_pLastReceivedInvite);
         sendMessage(sipResponse);
         m_rStateContext.m_bCallDisconnecting = TRUE;
         startByeTimeoutTimer();
      }
      else
      {
         // unexpected state
         return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, NULL);
      }
   }

   result = OS_SUCCESS;
   return NULL;
}

void BaseSipConnectionState::getSipDialogId(UtlString& sipCallId,
                                            UtlString& localTag,
                                            UtlString& remoteTag,
                                            UtlBoolean& isFromLocal)
{
   {
      OsReadLock lock(m_rStateContext);
      m_rStateContext.m_sipDialog.getCallId(sipCallId);
      m_rStateContext.m_sipDialog.getLocalTag(localTag);
      m_rStateContext.m_sipDialog.getRemoteTag(remoteTag);
      isFromLocal = m_rStateContext.m_sipDialog.isLocalInitiatedDialog();
   }
}

int BaseSipConnectionState::getNextLocalCSeq()
{
   OsWriteLock lock(m_rStateContext);
   return m_rStateContext.m_sipDialog.getNextLocalCseq();
}

int BaseSipConnectionState::getRandomCSeq() const
{
   UtlRandom randomGenerator;
   return (abs(randomGenerator.rand()) % 65535);
}

void BaseSipConnectionState::trackTransactionRequest(const SipMessage& sipMessage)
{
   int cseqNum;
   UtlString cseqMethod;
   sipMessage.getCSeqField(cseqNum, cseqMethod);

   CpSipTransactionManager& transactionManager = sipMessage.isFromThisSide() ? getClientTransactionManager() : getServerTransactionManager();

   if (needsTransactionTracking(cseqMethod))
   {
      CpSipTransactionManager::TransactionState transactionState = transactionManager.getTransactionState(cseqNum);

      if (transactionState == CpSipTransactionManager::TRANSACTION_NOT_FOUND)
      {
         // not found, start new transaction
         if (cseqMethod.compareTo(SIP_INVITE_METHOD) == 0)
         {
            ISipConnectionState::StateEnum connectionState = getCurrentState();
            if (connectionState == ISipConnectionState::CONNECTION_ESTABLISHED)
            {
               // this must be a re-INVITE
               transactionManager.startReInviteTransaction(cseqNum);
            }
            else
            {
               // this must be the initial INVITE
               transactionManager.startInitialInviteTransaction(cseqNum);
            }
         }
         else
         {
            transactionManager.startTransaction(cseqMethod, cseqNum);
         }
      }
   }

   if (cseqMethod.compareTo(SIP_ACK_METHOD) == 0)
   {
      // inbound ACK terminates some inbound INVITE transaction, outbound ACK terminates outbound INVITE transaction
      UtlBoolean res = transactionManager.endTransaction(SIP_INVITE_METHOD, cseqNum);
      if (res && m_rStateContext.m_sdpNegotiation.isInSdpNegotiation(sipMessage))
      {
         // INVITE transaction that is in SDP negotiation, reset SDP negotiation
         m_rStateContext.m_sdpNegotiation.resetSdpNegotiation();
      }
   }
}

void BaseSipConnectionState::trackTransactionResponse(const SipMessage& sipMessage)
{
   int cseqNum;
   UtlString cseqMethod;
   sipMessage.getCSeqField(cseqNum, cseqMethod);
   int statusCode = sipMessage.getResponseStatusCode();

   if (needsTransactionTracking(cseqMethod))
   {
      CpSipTransactionManager& transactionManager = sipMessage.isFromThisSide() ? getServerTransactionManager() : getClientTransactionManager();

      if (cseqMethod.compareTo(SIP_INVITE_METHOD) == 0 &&
          statusCode >= SIP_2XX_CLASS_CODE && statusCode < SIP_3XX_CLASS_CODE)
      {
         // INVITE transaction ends with ACK for 2xx code, since it is us who sends the ACK
         // therefore do nothing here
      }
      else if (statusCode >= SIP_2XX_CLASS_CODE)
      {
         // also handles INVITE 3xx and greater responses - terminate transaction now, 
         // since ACK is sent automatically by transaction layer and we will never see it
         UtlBoolean res = transactionManager.endTransaction(cseqMethod, cseqNum);
         if (res && (cseqMethod.compareTo(SIP_INVITE_METHOD) == 0 || cseqMethod.compareTo(SIP_UPDATE_METHOD) == 0) &&
             m_rStateContext.m_sdpNegotiation.isInSdpNegotiation(sipMessage))
         {
            // INVITE/UPDATE transaction that is in SDP negotiation, reset SDP negotiation
            m_rStateContext.m_sdpNegotiation.resetSdpNegotiation();
         }
      }
      // 1xx don't end transaction
   }
}

CpSipTransactionManager::InviteTransactionState BaseSipConnectionState::getInviteTransactionState() const
{
   CpSipTransactionManager::InviteTransactionState inviteState = getClientInviteTransactionState();
   if (inviteState == CpSipTransactionManager::INVITE_INACTIVE)
   {
      inviteState = getServerInviteTransactionState();
   }

   return inviteState;
}

CpSipTransactionManager::InviteTransactionState BaseSipConnectionState::getServerInviteTransactionState() const
{
   CpSipTransactionManager::InviteTransactionState inviteState = getServerTransactionManager().getInviteTransactionState();
   return inviteState;
}

CpSipTransactionManager::InviteTransactionState BaseSipConnectionState::getClientInviteTransactionState() const
{
   CpSipTransactionManager::InviteTransactionState inviteState = getClientTransactionManager().getInviteTransactionState();
   return inviteState;
}

void BaseSipConnectionState::startSessionRenegotiationTimer(ScReInviteTimerMsg::ReInviteReason reason,
                                                            UtlBoolean bCleanStart)
{
   deleteSessionRenegotiationTimer();

   UtlString sipCallId;
   UtlString localTag;
   UtlString remoteTag;
   UtlBoolean isFromLocal;

   getSipDialogId(sipCallId, localTag, remoteTag, isFromLocal);
   ScReInviteTimerMsg msg(reason, sipCallId, localTag, remoteTag, isFromLocal);
   OsTimerNotification* pNotification = new OsTimerNotification(m_rMessageQueueProvider.getLocalQueue(), msg);
   m_rStateContext.m_pSessionRenegotiationTimer = new OsTimer(pNotification);
   OsTime timerTime(T1_PERIOD_MSEC); // try again in 500ms
   m_rStateContext.m_pSessionRenegotiationTimer->oneshotAfter(timerTime); // start timer

   if (bCleanStart)
   {
      m_rStateContext.m_iRenegotiationRetryCount = 0;
   }
   else
   {
      m_rStateContext.m_iRenegotiationRetryCount++;
   }
}

void BaseSipConnectionState::deleteSessionRenegotiationTimer()
{
   if (m_rStateContext.m_pSessionRenegotiationTimer)
   {
      delete m_rStateContext.m_pSessionRenegotiationTimer;
      m_rStateContext.m_pSessionRenegotiationTimer = NULL;
   }
}

UtlBoolean BaseSipConnectionState::isUpdateActive()
{
   int updateCount = 0;
   updateCount += getServerTransactionManager().getTransactionCount(SIP_UPDATE_METHOD);
   updateCount += getClientTransactionManager().getTransactionCount(SIP_UPDATE_METHOD);

   return updateCount != 0;
}

UtlBoolean BaseSipConnectionState::mayRenegotiateMediaSession()
{
   ISipConnectionState::StateEnum connectionState = getCurrentState();

   if (connectionState != ISipConnectionState::CONNECTION_DISCONNECTED &&
       connectionState != ISipConnectionState::CONNECTION_UNKNOWN)
   {
      CpSipTransactionManager::InviteTransactionState inviteState = getInviteTransactionState();
      if (inviteState == CpSipTransactionManager::INVITE_INACTIVE)
      {
         // no invite is running, check for UPDATE
         if (!isUpdateActive())
         {
            // also no UPDATE, we may renegotiate
            return TRUE;
         }
      }
   }

   return FALSE;
}

void BaseSipConnectionState::doHold()
{
   // start hold via re-INVITE
   m_rStateContext.m_bUseLocalHoldSDP = TRUE;
   renegotiateMediaSession();
}

void BaseSipConnectionState::doUnhold()
{
   // start unhold via re-INVITE
   m_rStateContext.m_bUseLocalHoldSDP = FALSE;
   renegotiateMediaSession();
}

void BaseSipConnectionState::renegotiateMediaSession()
{
   if (m_rStateContext.m_bSdpRenegotiationUseUpdate && isMethodAllowed(SIP_UPDATE_METHOD))
   {
      sendUpdate();
   }
   else
   {
      sendReInvite();
   }
}

void BaseSipConnectionState::setLocalMediaConnectionState(SipConnectionStateContext::MediaConnectionState state,
                                                          UtlBoolean bRefreshMediaSessionState)
{
   m_rStateContext.m_localMediaConnectionState = state;
   if (bRefreshMediaSessionState)
   {
      refreshMediaSessionState();
   }
}

void BaseSipConnectionState::setRemoteMediaConnectionState(SipConnectionStateContext::MediaConnectionState state,
                                                           UtlBoolean bRefreshMediaSessionState)
{
   m_rStateContext.m_remoteMediaConnectionState = state;
   if (bRefreshMediaSessionState)
   {
      refreshMediaSessionState();
   }
}

void BaseSipConnectionState::refreshMediaSessionState()
{
   SipConnectionStateContext::MediaConnectionState localState = m_rStateContext.m_localMediaConnectionState;
   SipConnectionStateContext::MediaConnectionState remoteState = m_rStateContext.m_remoteMediaConnectionState;

   if (localState == SipConnectionStateContext::MEDIA_CONNECTION_NONE &&
       remoteState == SipConnectionStateContext::MEDIA_CONNECTION_NONE)
   {
      // session is not active
      m_rStateContext.m_mediaSessionState = SipConnectionStateContext::MEDIA_SESSION_NONE;
   }
   else if (localState == SipConnectionStateContext::MEDIA_CONNECTION_HELD &&
            remoteState == SipConnectionStateContext::MEDIA_CONNECTION_ACTIVE)
   {
      // local held, remote active -> local hold
      m_rStateContext.m_mediaSessionState = SipConnectionStateContext::MEDIA_SESSION_LOCALLY_HELD;
   }
   else if (localState == SipConnectionStateContext::MEDIA_CONNECTION_ACTIVE &&
            remoteState == SipConnectionStateContext::MEDIA_CONNECTION_HELD)
   {
      // local active, remote held -> remote hold
      m_rStateContext.m_mediaSessionState = SipConnectionStateContext::MEDIA_SESSION_REMOTELY_HELD;
   }
   else if (localState == SipConnectionStateContext::MEDIA_CONNECTION_HELD &&
            remoteState == SipConnectionStateContext::MEDIA_CONNECTION_HELD)
   {
      // local held, remote held -> full hold
      m_rStateContext.m_mediaSessionState = SipConnectionStateContext::MEDIA_SESSION_FULLY_HELD;
   }
   else if (localState == SipConnectionStateContext::MEDIA_CONNECTION_ACTIVE ||
            remoteState == SipConnectionStateContext::MEDIA_CONNECTION_ACTIVE)
   {
      // one of connections is active -> session is active
      m_rStateContext.m_mediaSessionState = SipConnectionStateContext::MEDIA_SESSION_ACTIVE;
   }
   else
   {
      m_rStateContext.m_mediaSessionState = SipConnectionStateContext::MEDIA_SESSION_NONE;
   }

   ISipConnectionState::StateEnum connectionState = getCurrentState();
   if (connectionState == ISipConnectionState::CONNECTION_ESTABLISHED)
   {
      fireMediaSessionEvents(FALSE, FALSE);
   }

   m_rStateContext.m_previousMediaSessionState = m_rStateContext.m_mediaSessionState;
}

void BaseSipConnectionState::fireMediaSessionEvents(UtlBoolean bForce, UtlBoolean bSupressConnected)
{
   if ((m_rStateContext.m_previousMediaSessionState != m_rStateContext.m_mediaSessionState) ||
       bForce)
   {
      // if established then also fire event for hold/remote hold
      switch (m_rStateContext.m_mediaSessionState)
      {
      case SipConnectionStateContext::MEDIA_SESSION_ACTIVE:
         if (!bSupressConnected)
         {
            m_rSipConnectionEventSink.fireSipXCallEvent(CP_CALLSTATE_CONNECTED, CP_CALLSTATE_CAUSE_NORMAL);
         }
         break;
      case SipConnectionStateContext::MEDIA_SESSION_REMOTELY_HELD:
         m_rSipConnectionEventSink.fireSipXCallEvent(CP_CALLSTATE_REMOTE_HELD, CP_CALLSTATE_CAUSE_NORMAL);
         break;
      case SipConnectionStateContext::MEDIA_SESSION_LOCALLY_HELD:
      case SipConnectionStateContext::MEDIA_SESSION_FULLY_HELD:
         m_rSipConnectionEventSink.fireSipXCallEvent(CP_CALLSTATE_HELD, CP_CALLSTATE_CAUSE_NORMAL);
         break;
      case SipConnectionStateContext::MEDIA_SESSION_NONE:
      default:
         // don't fire events
         break;
      }
   }
}

void BaseSipConnectionState::prepareSipRequest(SipMessage& sipRequest, const UtlString& method, int cseqNum)
{
   OsWriteLock lock(m_rStateContext);
   m_rStateContext.m_sipDialog.setRequestData(sipRequest, method, cseqNum);
}

SipConnectionStateTransition* BaseSipConnectionState::handleInviteRedirectResponse(const SipMessage& sipMessage)
{
   int responseCode = sipMessage.getResponseStatusCode();
   UtlString responseText;
   sipMessage.getResponseStatusText(&responseText);

   UtlString contactUri;
   int index = 0;

   // go through all contacts
   while (sipMessage.getContactUri(index++, &contactUri))
   {
      m_rStateContext.m_redirectContactList.append(contactUri.clone()); // append duplicate uri
   }

   if (m_rStateContext.m_redirectContactList.entries() > 0)
   {
      // we are redirecting
      m_rStateContext.m_bRedirecting = TRUE;
      return followNextRedirect();
   }
   else
   {
      // no contacts present, give up
      GeneralTransitionMemory memory(CP_CALLSTATE_CAUSE_REDIRECTED, responseCode, responseText);
      return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
   }
}

SipConnectionStateTransition* BaseSipConnectionState::followNextRedirect()
{
   if (m_rStateContext.m_redirectContactList.entries() > 0)
   {
      Url fromField;
      Url toField;
      UtlString sipCallId;
      UtlString contactUrl(getLocalContactUrl());
      // get next contact, and construct new INVITE
      {
         OsReadLock lock(m_rStateContext);
         m_rStateContext.m_sipDialog.getLocalField(fromField);
         m_rStateContext.m_sipDialog.getRemoteField(toField);
         m_rStateContext.m_sipDialog.getCallId(sipCallId);
      }
      // keep tag of from field, but remove it from toField
      toField.removeFieldParameters();
      int cseqNum = getRandomCSeq();

      SipMessage sipInvite;
      sipInvite.setSecurityAttributes(m_rStateContext.m_pSecurity);
      sipInvite.setInviteData(fromField.toString(), toField.toString(),
         NULL, contactUrl, sipCallId,
         cseqNum, m_rStateContext.m_sessionTimerProperties.getSessionExpires());

      UtlString* pRedirectContactUri = dynamic_cast<UtlString*>(m_rStateContext.m_redirectContactList.get());
      if (pRedirectContactUri)
      {
         sipInvite.changeUri(*pRedirectContactUri); // for redirect, change just the URI, not toField

         if (!m_rStateContext.m_locationHeader.isNull())
         {
            sipInvite.setLocationField(m_rStateContext.m_locationHeader);
         }

         initializeSipDialog(sipInvite); // restart sip dialog

         // add SDP if negotiation mode is immediate, otherwise don't add it
         if (m_rStateContext.m_sdpNegotiation.getSdpOfferingMode() == CpSdpNegotiation::SDP_OFFERING_IMMEDIATE)
         {
            if (!prepareSdpOffer(sipInvite))
            {
               // SDP negotiation start failed
               // media connection creation failed
               GeneralTransitionMemory memory(CP_CALLSTATE_CAUSE_RESOURCE_LIMIT);
               return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
            }
         }

         // try to send sip message
         UtlBoolean sendSuccess = sendMessage(sipInvite);
         if (!sendSuccess)
         {
            GeneralTransitionMemory memory(CP_CALLSTATE_CAUSE_NETWORK);
            return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
         }
      }
   }

   // no contacts present, give up
   GeneralTransitionMemory memory(CP_CALLSTATE_CAUSE_REDIRECTED);
   return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
