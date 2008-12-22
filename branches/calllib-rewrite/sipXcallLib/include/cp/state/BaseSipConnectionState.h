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

#ifndef BaseSipConnectionState_h__
#define BaseSipConnectionState_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsStatus.h>
#include <utl/UtlString.h>
#include <cp/CpDefs.h>
#include <cp/state/ISipConnectionState.h>
#include <cp/state/SipConnectionStateContext.h>
#include <cp/CpNatTraversalConfig.h>

// DEFINES
#define MAX_ADDRESS_CANDIDATES      12

// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class SipUserAgent;
class CpMediaInterfaceProvider;
class CpMessageQueueProvider;
class XSipConnectionEventSink;
class SipConnectionStateTransition;
class StateTransitionMemory;
class SipMessage;
class ScTimerMsg;
class ScCommandMsg;
class ScNotificationMsg;
class Sc100RelTimerMsg;
class Sc2xxTimerMsg;
class ScDisconnectTimerMsg;
class ScReInviteTimerMsg;
class ScByeRetryTimerMsg;

/**
 * Parent to all concrete sip connection states. Should be used for handling
 * common to all states. This should be used as the last resort handler, usually
 * responding with errors to requests.
 */
class BaseSipConnectionState : public ISipConnectionState
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /** Constructor. */
   BaseSipConnectionState(SipConnectionStateContext& rStateContext,
                          SipUserAgent& rSipUserAgent,
                          CpMediaInterfaceProvider& rMediaInterfaceProvider,
                          CpMessageQueueProvider& rMessageQueueProvider,
                          XSipConnectionEventSink& rSipConnectionEventSink,
                          const CpNatTraversalConfig& natTraversalConfig);

   /** Copy constructor. */
   BaseSipConnectionState(const BaseSipConnectionState& rhs);

   /** Destructor. */
   virtual ~BaseSipConnectionState();

   /* ============================ MANIPULATORS ============================== */

   /**
    * Default state entry handler.
    */
   virtual void handleStateEntry(StateEnum previousState, const StateTransitionMemory* pTransitionMemory);

   /**
    * Default state exit handler.
    */
   virtual void handleStateExit(StateEnum nextState, const StateTransitionMemory* pTransitionMemory);

   /**
   * Handles SipMessageEvent, which can be inbound sip request/response or error
   * sending status.
   * If instance cannot handle this event, it must pass it to parent as the last resort.
   *
   * @param rEvent Instance of SipMessageEvent that needs to be handled.
   * @return Instance of SipConnectionStateTransition if a transition should occur. NULL if no transition should occur.
   */
   virtual SipConnectionStateTransition* handleSipMessageEvent(const SipMessageEvent& rEvent);

   /** Connects call to given address. Uses supplied sip call-id. */
   virtual SipConnectionStateTransition* connect(OsStatus& result,
                                                 const UtlString& sipCallId,
                                                 const UtlString& localTag,
                                                 const UtlString& toAddress,
                                                 const UtlString& fromAddress,
                                                 const UtlString& locationHeader,
                                                 CP_CONTACT_ID contactId);

   /** Disconnects call */
   virtual SipConnectionStateTransition* dropConnection(OsStatus& result);

   /** Put the specified terminal connection on hold. */
   virtual SipConnectionStateTransition* holdConnection(OsStatus& result);

   /** Convenience method to take the terminal connection off hold. */
   virtual SipConnectionStateTransition* unholdConnection(OsStatus& result);

   /** Handles timer message. */
   virtual SipConnectionStateTransition* handleTimerMessage(const ScTimerMsg& timerMsg);

   /** Handles CpMessageTypes::SC_COMMAND message. */
   virtual SipConnectionStateTransition* handleCommandMessage(const ScCommandMsg& rMsg);

   /** Handles CpMessageTypes::ScNotificationMsg message. */
   virtual SipConnectionStateTransition* handleNotificationMessage(const ScNotificationMsg& rMsg);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   /**
   * Constructs transition from current state into given destination state.
   * @pTansitionMemory Optional memory object which should be supplied to state transition. Local copy
   * will be made.
   */
   virtual SipConnectionStateTransition* getTransition(ISipConnectionState::StateEnum nextState,
                                                       const StateTransitionMemory* pTansitionMemory) const = 0;

   /**
   * Constructs transition from current state into given destination state.
   * @pTansitionMemory Optional memory object which should be supplied to state transition. Local copy
   * will be made.
   */
   virtual SipConnectionStateTransition* getTransitionObject(BaseSipConnectionState* pDestination,
                                                             const StateTransitionMemory* pTansitionMemory) const;

   /** Updates state of SipDialog with given SipMessage. Use for both inbound and outbound messages. */
   void updateSipDialog(const SipMessage& sipMessage);

   /** Initializes SipDialog */
   void initializeSipDialog(const SipMessage& sipMessage);

   /** Sends specified sip message, updating the state of SipDialog. SipMessage will be updated. */
   UtlBoolean sendMessage(SipMessage& sipMessage);

   /** Handles SIP request and response messages. */
   virtual SipConnectionStateTransition* processSipMessage(const SipMessage& sipMessage);

   /**
    * Processes inbound SIP request message. This method calls special handlers for each supported
    * method that should be overridden in concrete state.
    */
   virtual SipConnectionStateTransition* processRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP re-INVITE requests. It won't handle initial INVITEs. */
   virtual SipConnectionStateTransition* processInviteRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP UPDATE requests */
   virtual SipConnectionStateTransition* processUpdateRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP ACK requests. Processes initial INVITES and re-INVITEs */
   virtual SipConnectionStateTransition* processAckRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP BYE requests */
   virtual SipConnectionStateTransition* processByeRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP CANCEL requests */
   virtual SipConnectionStateTransition* processCancelRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP INFO requests */
   virtual SipConnectionStateTransition* processInfoRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP NOTIFY requests */
   virtual SipConnectionStateTransition* processNotifyRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP REFER requests */
   virtual SipConnectionStateTransition* processReferRequest(const SipMessage& sipMessage);

   /** Handles inbound SIP PRACK requests */
   virtual SipConnectionStateTransition* processPrackRequest(const SipMessage& sipMessage);

   /**
    * Processes inbound SIP response message. This is the default handler than needs to
    * be called first, if this method is overridden. If this method handles the response,
    * and returns new transition, overriding method or caller must not continue processing,
    * but return the same transition.
    *
    * Implementation of this method is based on RFC5057. This method is responsible for
    * transition to disconnected state if some error response is received.
    *
    * This method handles error responses in a method independent way, terminating transaction
    * or dialog if needed. Response codes 1xx, 2xx and 3xx are expected to be handled by
    * method specific handlers. ACK doesn't have response.
    */
   virtual SipConnectionStateTransition* processResponse(const SipMessage& sipMessage);

   /** Handles non fatal inbound SIP responses. These do not terminate the dialog. */
   virtual SipConnectionStateTransition* processNonFatalResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP INVITE responses */
   virtual SipConnectionStateTransition* processInviteResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP UPDATE responses */
   virtual SipConnectionStateTransition* processUpdateResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP BYE responses */
   virtual SipConnectionStateTransition* processByeResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP CANCEL responses */
   virtual SipConnectionStateTransition* processCancelResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP INFO responses */
   virtual SipConnectionStateTransition* processInfoResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP OPTIONS responses */
   virtual SipConnectionStateTransition* processOptionsResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP NOTIFY responses */
   virtual SipConnectionStateTransition* processNotifyResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP REFER responses */
   virtual SipConnectionStateTransition* processReferResponse(const SipMessage& sipMessage);

   /** Handles inbound SIP PRACK responses */
   virtual SipConnectionStateTransition* processPrackResponse(const SipMessage& sipMessage);

   /** Called when authentication retry occurs. We need to update dialogs. */
   virtual SipConnectionStateTransition* handleAuthenticationRetryEvent(const SipMessage& sipMessage);

   /** Returns TRUE if some SIP method is allowed (may be sent) */
   UtlBoolean isMethodAllowed(const UtlString& sMethod);

   /** Deletes media connection if it exists, stopping remote and local audio */
   void deleteMediaConnection();

   /** Terminates sip dialog */
   void terminateSipDialog();

   /** TRUE if dialog was initiated locally */
   UtlBoolean isLocalInitiatedDialog();

   /** Gets the outbound transaction manager */
   CpSipTransactionManager& getOutTransactionManager() const;

   /** Gets the inbound transaction manager */
   CpSipTransactionManager& getInTransactionManager() const;

   /** Gets ID of media connection */
   int getMediaConnectionId() const;

   /** Sets ID of media connection */
   void setMediaConnectionId(int mediaConnectionId);

   /** Handles 100Rel timer message. */
   virtual SipConnectionStateTransition* handle100RelTimerMessage(const Sc100RelTimerMsg& timerMsg);

   /** Handles 2xx timer message. */
   virtual SipConnectionStateTransition* handle2xxTimerMessage(const Sc2xxTimerMsg& timerMsg);

   /** Handles disconnect timer message. */
   virtual SipConnectionStateTransition* handleDisconnectTimerMessage(const ScDisconnectTimerMsg& timerMsg);

   /** Handles re-invite timer message. */
   virtual SipConnectionStateTransition* handleReInviteTimerMessage(const ScReInviteTimerMsg& timerMsg);

   /** Handles hold re-invite timer message. */
   virtual SipConnectionStateTransition* handleHoldTimerMessage(const ScReInviteTimerMsg& timerMsg);

   /** Handles unhold re-invite timer message. */
   virtual SipConnectionStateTransition* handleUnholdTimerMessage(const ScReInviteTimerMsg& timerMsg);

   /** Handles bye retry timer message. */
   virtual SipConnectionStateTransition* handleByeRetryTimerMessage(const ScByeRetryTimerMsg& timerMsg);

   /** Quick access to sip call-id */
   UtlString getCallId() const;

   /** Checks if given sip method needs transaction tracking */
   UtlBoolean needsTransactionTracking(const UtlString& sipMethod) const;

   /** 
    * Builds contact URL based on contactId. Might be sips if TLS transport is selected. UserId, display name
    * are filled from fromAddress.
    */
   UtlString buildContactUrl(const Url& fromAddress) const;

   /** 
    * Gets local contact URL from SipDialog. Can only be used once SipDialog has been initialized with first
    * outbound request. Contact Url will have display name if its available.
    */
   void getLocalContactUrl(Url& contactUrl);

   /**
    * Gets local contact URL as string.
    */
   UtlString getLocalContactUrl();

   /** Builds default contact URL. URL will not be sips, and will contain UserId, display name from fromAddress*/
   UtlString buildDefaultContactUrl(const Url& fromAddress) const;

   /** Changes scheme to sips: if secured transport is used based on contactId */
   void secureUrl(Url& fromAddress) const;

   /** Creates media connection from media interface, returning mediaConnectionId */
   UtlBoolean setupMediaConnection(RTP_TRANSPORT rtpTransportOptions, int& mediaConnectionId);

   /** Starts SDP negotiation, adds SDP offer to given SipMessage and creates a media connection if it doesn't exist */
   UtlBoolean prepareSdpOffer(SipMessage& sipMessage);

   /**
    * handles inbound SDP offer, starts SDP negotiation, creating media connection if it doesn't exist
    * Returns FALSE if an error occurred and connection needs to be disconnected.
    */
   UtlBoolean handleSdpOffer(const SipMessage& sipMessage);

   /** adds SDP offer to given SipMessage */
   UtlBoolean prepareSdpAnswer(SipMessage& sipMessage);

   /**
    * handles inbound SDP answer, setting media connection destination
    * Returns FALSE if an error occurred and connection needs to be disconnected.
    */
   UtlBoolean handleSdpAnswer(const SipMessage& sipMessage);

   /** Handles remote SDP body, finds out if there is match between our local codecs and received codecs */
   UtlBoolean handleRemoteSdpBody(const SdpBody& sdpBody);

   /** Commits changes negotiated by SDP offer/answer, starting/stopping RTP flow */
   UtlBoolean commitMediaSessionChanges();

   /** Sets last sent invite */
   void setLastSentInvite(const SipMessage& sipMessage);

   /** Sets last received invite */
   void setLastReceivedInvite(const SipMessage& sipMessage);

   /** Sets last sent 2xx response to invite */
   void setLastSent2xxToInvite(const SipMessage& sipMessage);

   /** Gets session timer properties */
   CpSessionTimerProperties& getSessionTimerProperties();

   /** Handles 422 INVITE response */
   void handleSmallInviteSessionInterval(const SipMessage& sipMessage);

   /** Sends options request */
   void sendOptionsRequest();

   /** Checks if we know Allow of remote side, and optionally sends options if we don't */
   void checkRemoteAllow();

   /** Sends ACK to given 200 OK response */
   void handleInvite2xxResponse(const SipMessage& sipResponse);

   /** Sends BYE to terminate call */
   void sendBye();

   /** Sends CANCEL to terminate call */
   void sendInviteCancel();

   /** Sends re-INVITE for hold/unhold/codec renegotiation */
   void sendReInvite();

   /** Sends UPDATE for hold/unhold/codec renegotiation */
   void sendUpdate();

   /**
    * Sets media connection destination to given host/ports if ICE is disabled, or to all
    * candidate addresses in SDP body if ICE is enabled.
    */
   void setMediaDestination(const char* hostAddress, ///< remote RTP address
                            int audioRtpPort, ///< remote RTP audio port
                            int audioRtcpPort, ///< remote RTPC audio port
                            int videoRtpPort,
                            int videoRtcpPort,
                            const SdpBody* pRemoteBody);

   /** Disconnects call. CANCEL should be used if 200 OK was not yet received */
   SipConnectionStateTransition* doByeConnection(OsStatus& result);

   /** Disconnects outbound call using CANCEL. Cannot be used on inbound call */
   SipConnectionStateTransition* doCancelConnection(OsStatus& result);

   /** Sends immediate request to call to destroy this connection. */
   void requestConnectionDestruction();

   /** Starts cancel timer for force dropping connection */
   void startCancelTimeoutTimer();

   /** Deletes cancel timer */
   void deleteCancelTimeoutTimer();

   /** Starts bye timer for force dropping connection */
   void startByeTimeoutTimer();

   /** Deletes bye timer */
   void deleteByeTimeoutTimer();

   /** 
    * Starts bye timer for trying to send BYE later. For inbound call we may not send BYE
    * after 200 OK unless we receive ACK. Therefore we wait a little bit, and if ACK is not received
    * then drop the call. If drop was requested we do not want to wait until 200 OK resending gives
    * a timeout.
    */
   void startByeRetryTimer();

   /** Deletes delayed bye timer */
   void deleteByeRetryTimer();

   /** Starts timer to retransmit 2xx messages until ack is received */
   void start2xxRetransmitTimer();

   /** Deletes timer responsible for retransmit of 2xx invite responses */
   void delete2xxRetransmitTimer();

   /** Rejects inbound connection that is in progress (not yet established by sending 403 Forbidden */
   SipConnectionStateTransition* doRejectInboundConnectionInProgress(OsStatus& result);

   /** Gets Id of sip dialog */
   void getSipDialogId(UtlString& sipCallId,
                       UtlString& localTag,
                       UtlString& remoteTag,
                       UtlBoolean& isFromLocal);

   /** Gets next local CSeq number */
   int getNextLocalCSeq();

   /** Gets random CSeq number for bootstrap */
   int getRandomCSeq() const;

   /** Must be called for inbound requests to track inbound transactions */
   void trackInboundTransactionRequest(const SipMessage& sipMessage);

   /** Must be called for outbound responses to track inbound transactions */
   void trackInboundTransactionResponse(const SipMessage& sipMessage);

   /**
    * Gets state of invite transaction. Considers both outbound and inbound invite transactions.
    */
   CpSipTransactionManager::InviteTransactionState getInviteTransactionState() const;

   /** Gets state of inbound invite transaction. */
   CpSipTransactionManager::InviteTransactionState getInInviteTransactionState() const;

   /** Gets state of outbound invite transaction. */
   CpSipTransactionManager::InviteTransactionState getOutInviteTransactionState() const;

   /** Starts hold/unhold timer to execute the action later */
   void startHoldTimer(UtlBoolean bHold = TRUE);

   /** Deletes hold/unhold timer */
   void deleteHoldTimer();

   /** Returns TRUE if some UPDATE is active. */
   UtlBoolean isUpdateActive();

   /** Returns TRUE if we may start renegotiating media session now */
   UtlBoolean mayRenegotiateMediaSession();

   /** Terminates inbound or outbound invite transaction regardless of cseq number */
   void endInviteTransaction();

   /** Terminates inbound or outbound invite transaction, taking into account cseq number */
   void endInviteTransaction(UtlBoolean bIsOutboundTransaction, int cseqNumber);

   /** Initiates hold via re-INVITE or UPDATE */
   UtlBoolean doHold();

   /** Initiates unhold via re-INVITE or UPDATE */
   UtlBoolean doUnhold();

   /** Renegotiates media session */
   void renegotiateMediaSession();

   /** Sets state of local media connection. Don't update state directly. Use this function. */
   void setLocalMediaConnectionState(SipConnectionStateContext::MediaConnectionState state,
                                     UtlBoolean bRefreshMediaSessionState = TRUE);

   /** Sets state of remote media connection. Don't update state directly. Use this function. */
   void setRemoteMediaConnectionState(SipConnectionStateContext::MediaConnectionState state,
                                      UtlBoolean bRefreshMediaSessionState = TRUE);

   /** Refreshes state of media session. Must be called after media connection state is changed */
   void refreshMediaSessionState();

   /** Fires media session events, to notify sipxtapi about held/active/remote held call states */
   void fireMediaSessionEvents(UtlBoolean bForce = FALSE, UtlBoolean bSupressConnected = FALSE);

   SipConnectionStateContext& m_rStateContext; ///< context containing state of sip connection. Needs to be locked when accessed.
   SipUserAgent& m_rSipUserAgent; // for sending sip messages
   CpMediaInterfaceProvider& m_rMediaInterfaceProvider; ///< media interface provider
   CpMessageQueueProvider& m_rMessageQueueProvider; ///< message queue provider
   XSipConnectionEventSink& m_rSipConnectionEventSink; ///< event sink (router) for various sip connection event types
   CpNatTraversalConfig m_natTraversalConfig; ///< NAT traversal configuration

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   /** Private assignment operator */
   BaseSipConnectionState& operator=(const BaseSipConnectionState& rhs);
};

#endif // BaseSipConnectionState_h__
