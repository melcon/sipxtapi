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
class ScDisconnectTimerMsg;
class ScReInviteTimerMsg;

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
   virtual SipConnectionStateTransition* connect(const UtlString& toAddress,
                                                 const UtlString& fromAddress,
                                                 const UtlString& locationHeader,
                                                 CP_CONTACT_ID contactId);

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

   /** Sends specified sip message, updating the state of SipDialog. SipMessage will be updated. */
   UtlBoolean sendMessage(SipMessage& sipMessage);

   /**
    * Processes inbound SIP response message. This is the default handler than needs to
    * be called first, if this method is overridden. If this method handles the response,
    * and returns new transition, overriding method or caller must not continue processing,
    * but return the same transition.
    *
    * Implementation of this method is based on RFC5057. This method is responsible for
    * transition to disconnected state if some error response is received.
    */
   virtual SipConnectionStateTransition* processResponse(const SipMessage& sipMessage);

   /** Returns TRUE if some SIP method is allowed (may be sent) */
   UtlBoolean isMethodAllowed(const UtlString& sMethod);

   /** Returns TRUE if an INVITE transaction is active */
   UtlBoolean isInviteTransactionActive() const;

   /** Starts initial INVITE transaction. We can allow only 1 INVITE transaction at time, so we need to track it manually. */
   void startInviteTransaction();

   /**
   * Starts re-INVITE transaction.
   *
   * @param bIsSessionRefresh TRUE for session timer initiated refresh. This must succeed,
   * otherwise call must be dropped. Normal re-INVITE failure can be ignored.
   */
   void startReInviteTransaction(UtlBoolean bIsSessionRefresh = FALSE);

   /**
    * Stops INVITE transaction. This doesn't terminate the transaction in transaction manager.
    * Perhaps unify with transaction manager.
    */
   void stopInviteTransaction();

   /** Deletes media connection if it exists, stopping remote and local audio */
   void deleteMediaConnection();

   /** Terminates sip dialog */
   void terminateSipDialog();

   /** Gets the transaction manager */
   CpSipTransactionManager& getTransactionManager() const;

   /** Gets ID of media connection */
   int getMediaConnectionId() const;

   /** Sets ID of media connection */
   void setMediaConnectionId(int mediaConnectionId);

   /** Handles 100Rel timer message. */
   virtual SipConnectionStateTransition* handle100RelTimerMessage(const Sc100RelTimerMsg& timerMsg);

   /** Handles 100Rel timer message. */
   virtual SipConnectionStateTransition* handleDisconnectTimerMessage(const ScDisconnectTimerMsg& timerMsg);

   /** Handles 100Rel timer message. */
   virtual SipConnectionStateTransition* handleReInviteTimerMessage(const ScReInviteTimerMsg& timerMsg);

   /** Quick access to sip call-id */
   UtlString getCallId() const;

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
