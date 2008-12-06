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
#include <cp/CpMediaInterfaceProvider.h>
#include <cp/msg/ScTimerMsg.h>
#include <cp/msg/ScCommandMsg.h>
#include <cp/msg/ScNotificationMsg.h>
#include <cp/msg/Sc100RelTimerMsg.h>
#include <cp/msg/ScDisconnectTimerMsg.h>
#include <cp/msg/ScReInviteTimerMsg.h>

// DEFINES
// CANCEL doesn't need transaction tracking
#define TRACKABLE_METHODS "INVITE UPDATE INFO NOTIFY REFER OPTIONS PRACK"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
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
            getTransactionManager().endInviteTransaction();
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

SipConnectionStateTransition* BaseSipConnectionState::connect(const UtlString& sipCallId,
                                                              const UtlString& localTag,
                                                              const UtlString& toAddress,
                                                              const UtlString& fromAddress,
                                                              const UtlString& locationHeader,
                                                              CP_CONTACT_ID contactId)
{
   // we reject connect in all states except for Dialing
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleTimerMessage(const ScTimerMsg& timerMsg)
{
   switch (timerMsg.getPayloadType())
   {
   case ScTimerMsg::PAYLOAD_TYPE_100REL:
      return handle100RelTimerMessage((const Sc100RelTimerMsg&)timerMsg);
   case ScTimerMsg::PAYLOAD_TYPE_DISCONNECT:
      return handleDisconnectTimerMessage((const ScDisconnectTimerMsg&)timerMsg);
   case ScTimerMsg::PAYLOAD_TYPE_REINVITE:
      return handleReInviteTimerMessage((const ScReInviteTimerMsg&)timerMsg);
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
   return m_rSipUserAgent.send(sipMessage);
}

SipConnectionStateTransition* BaseSipConnectionState::processSipMessage(const SipMessage& sipMessage)
{
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
   // process inbound sip message request
   UtlString method;
   sipMessage.getRequestMethod(&method);

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
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processAckRequest(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processByeRequest(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processCancelRequest(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processInfoRequest(const SipMessage& sipMessage)
{
   // TODO: Implement
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

      CpSipTransactionManager::TransactionState transactionState = getTransactionManager().getTransactionState(seqMethod, iSeqNumber);
      if (transactionState == CpSipTransactionManager::TRANSACTION_ACTIVE)
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
               getTransactionManager().endTransaction(seqMethod, iSeqNumber);
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
               getTransactionManager().endTransaction(seqMethod, iSeqNumber);
               SipResponseTransitionMemory memory(statusCode, statusText);
               return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
            }
            break;
         // these only affect transaction - if seqMethod was INVITE and media session doesn't exist, then destroy dialog
         // if media session exists, then no need to destroy the dialog - only INVITE failed for some reason, we can keep
         // the original media session
         default:
            // handle unknown 4XX, 5XX, 6XX
            CpSipTransactionManager::InviteTransactionState inviteState = getTransactionManager().getInviteTransactionState();
            if (statusCode >= SIP_4XX_CLASS_CODE && statusCode < SIP_7XX_CLASS_CODE &&
                seqMethod.compareTo(SIP_INVITE_METHOD) == 0 &&
                (inviteState == CpSipTransactionManager::INITIAL_INVITE_ACTIVE ||
                 inviteState == CpSipTransactionManager::REINVITE_SESSION_REFRESH_ACTIVE))
            {
               // failure during initial INVITE, terminate dialog
               getTransactionManager().endTransaction(seqMethod, iSeqNumber);
               SipResponseTransitionMemory memory(statusCode, statusText);
               return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
            }
            // otherwise re-INVITE failed
            ;
         }

         return processNonFatalResponse(sipMessage);
      }
      // if transaction was not found or is terminated then ignore response
   }

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
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processUpdateResponse(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processByeResponse(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processCancelResponse(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processInfoResponse(const SipMessage& sipMessage)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::processOptionsResponse(const SipMessage& sipMessage)
{
   // TODO: Implement
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
      if (sipMessage.isRequest())
      {
         int seqNum;
         UtlString seqMethod;
         sipMessage.getCSeqField(seqNum, seqMethod);

         if (needsTransactionTracking(seqMethod))
         {
            getTransactionManager().updateActiveTransaction(seqMethod, seqNum);
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
         pInterface->stopRtpReceive(mediaConnectionId);
         pInterface->stopRtpSend(mediaConnectionId);
         pInterface->deleteConnection(mediaConnectionId);
      }
   }
}

void BaseSipConnectionState::terminateSipDialog()
{
   getTransactionManager().endInviteTransaction();

   {
      OsWriteLock lock(m_rStateContext);
      m_rStateContext.m_sipDialog.terminateDialog();
   }
}

CpSipTransactionManager& BaseSipConnectionState::getTransactionManager() const
{
   return m_rStateContext.m_sipTransactionMgr;
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
}

SipConnectionStateTransition* BaseSipConnectionState::handle100RelTimerMessage(const Sc100RelTimerMsg& timerMsg)
{
   // TODO: implement handler
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleDisconnectTimerMessage(const ScDisconnectTimerMsg& timerMsg)
{
   // TODO: implement handler
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::handleReInviteTimerMessage(const ScReInviteTimerMsg& timerMsg)
{
   // TODO: implement handler
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
      SIPX_CONTACT_ADDRESS* pContact = m_rSipUserAgent.getContactDb().getLocalContact(m_rStateContext.m_contactId);
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
         return sContact;
      }
   }

   return buildDefaultContactUrl(fromUrl);
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
   SIPX_CONTACT_ADDRESS* pContact = m_rSipUserAgent.getContactDb().getLocalContact(m_rStateContext.m_contactId);
   if (pContact != NULL)
   {
      if (pContact->eTransportType == TRANSPORT_TLS)
      {
         fromUrl.setScheme(Url::SipsUrlScheme);
      }
   }

   delete pContact;
   pContact = NULL;
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
      OsWriteLock lock(m_rStateContext);
      m_rStateContext.m_mediaConnectionId = mediaConnectionId;
      return TRUE;
   }

   return FALSE;
}

UtlBoolean BaseSipConnectionState::startSdpNegotiation(SipMessage& sipMessage)
{
   m_rStateContext.m_sdpNegotiation.startSdpNegotiation(TRUE); // start locally initiated SDP negotiation
   CpMediaInterface* pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface();

   int mediaConnectionId = CpMediaInterface::INVALID_CONNECTION_ID;
   if (setupMediaConnection(m_rStateContext.m_rtpTransport, mediaConnectionId))
   {
      SIPX_CONTACT_ADDRESS* pContact = m_rSipUserAgent.getContactDb().getLocalContact(m_rStateContext.m_contactId);
      if (pContact != NULL)
      {
         pMediaInterface->setContactType(mediaConnectionId, pContact->eContactType, m_rStateContext.m_contactId);
      }
      else
      {
         pMediaInterface->setContactType(mediaConnectionId, (SIPX_CONTACT_TYPE)AUTOMATIC_CONTACT_TYPE, AUTOMATIC_CONTACT_ID);
      }

      delete pContact;
      pContact = NULL;

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

         m_rStateContext.m_sdpNegotiation.addSdpBody(sipMessage,
            nRtpContacts, hostAddresses, receiveRtpPorts, receiveRtcpPorts, receiveVideoRtpPorts, receiveVideoRtcpPorts,
            transportTypes, supportedCodecs, srtpParams, totalBandwidth, videoFramerate, m_rStateContext.m_rtpTransport);

         return TRUE;
      }
   }

   return FALSE;
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
