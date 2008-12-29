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

#ifndef SipConnectionStateMachine_h__
#define SipConnectionStateMachine_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
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
class ISipConnectionState;
class BaseSipConnectionState;
class SipMessageEvent;
class XSipConnectionContext;
class SipConnectionStateObserver;
class SipUserAgent;
class CpMediaInterfaceProvider;
class CpMessageQueueProvider;
class XSipConnectionEventSink;
class SipConnectionStateTransition;
class ScTimerMsg;
class ScCommandMsg;
class ScNotificationMsg;

/**
 * State machine handling various connection states.
 *
 * Not thread safe. Caller must ensure that only single thread will invoke
 * methods on this class.
 */
class SipConnectionStateMachine
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */
   
   /** Constructor. */
   SipConnectionStateMachine(SipUserAgent& rSipUserAgent,
                             CpMediaInterfaceProvider& rMediaInterfaceProvider,
                             CpMessageQueueProvider& rMessageQueueProvider,
                             XSipConnectionEventSink& rSipConnectionEventSink,
                             const CpNatTraversalConfig& natTraversalConfig);

   /** Destructor. */
   virtual ~SipConnectionStateMachine();

   /* ============================ MANIPULATORS ============================== */

   /**
    * Handles SipMessageEvent, which can be inbound SipMessage or notification about
    * send failure.
    */
   UtlBoolean handleSipMessageEvent(const SipMessageEvent& rEvent);

   /** Connects call to given address. Uses supplied sip call-id. */
   OsStatus connect(const UtlString& sipCallId,
                    const UtlString& localTag,
                    const UtlString& toAddress,
                    const UtlString& fromAddress,
                    const UtlString& locationHeader,
                    CP_CONTACT_ID contactId);

   /** 
   * Accepts inbound call connection.
   */
   OsStatus acceptConnection(const UtlString& locationHeader,
                             CP_CONTACT_ID contactId);

   /**
   * Reject the incoming connection.
   */
   OsStatus rejectConnection();

   /**
   * Redirect the incoming connection.
   */
   OsStatus redirectConnection(const UtlString& sRedirectSipUrl);

   /**
   * Answer the incoming terminal connection.
   */
   OsStatus answerConnection();

   /** Disconnects call */
   OsStatus dropConnection();

   /** Put the specified terminal connection on hold. */
   OsStatus holdConnection();

   /** Convenience method to take the terminal connection off hold. */
   OsStatus unholdConnection();

   /** Renegotiates media session codecs */
   OsStatus renegotiateCodecsConnection();

   /** Sends an INFO message to the other party(s) on the call */
   OsStatus sendInfo(const UtlString& sContentType,
                     const char* pContent,
                     const size_t nContentLength,
                     void* pCookie);

   /** Handles timer message */
   UtlBoolean handleTimerMessage(const ScTimerMsg& timerMsg);

   /** Handles CpMessageTypes::SC_COMMAND message */
   UtlBoolean handleCommandMessage(const ScCommandMsg& rMsg);

   /** Handles CpMessageTypes::ScNotificationMsg message */
   UtlBoolean handleNotificationMessage(const ScNotificationMsg& rMsg);

   /** Configures session timer properties */
   void configureSessionTimer(int sessionExpiration, CP_SESSION_TIMER_REFRESH sessionTimerRefresh);

   /** Configures SIP UPDATE usage */
   void configureUpdate(CP_SIP_UPDATE_CONFIG updateSetting);

   /** Configures 100rel (PRACK) support */
   void configure100rel(CP_100REL_CONFIG c100relSetting);

   /** Configures expiration time for INVITE requests. If no final response is received, INVITE is cancelled. */
   void configureInviteExpiration(int inviteExpiresSeconds);

   /* ============================ ACCESSORS ================================= */

   /**
    * Sets state observer which will be notified about state changes. Notifications
    * are always fired after state change occurs.
    */
   void setStateObserver(SipConnectionStateObserver* val) { m_pStateObserver = val; }

   /**
    * Gets current state code of state machine.
    */
   ISipConnectionState::StateEnum getCurrentState();

   /** Gets reference to public Sip connection context. Must be locked when modified. */
   XSipConnectionContext& getSipConnectionContext() const;

   /* ============================ INQUIRY =================================== */

   /** Gets state of media session */
   SipConnectionStateContext::MediaSessionState getMediaSessionState() const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   SipConnectionStateMachine(const SipConnectionStateMachine& rhs);

   SipConnectionStateMachine& operator=(const SipConnectionStateMachine& rhs);

   /**
    * Handles state changes. Responsible for deletion of previous state and state change
    * notifications. Doesn't attempt to delete passed transition object. Use for static
    * transition objects.
    */
   void handleStateTransition(SipConnectionStateTransition& rStateTransition);

   /** Handles state transition, including deletion of passed transition object */
   void handleStateTransition(SipConnectionStateTransition* pStateTransition);

   /** Notify observer that we entered new state */
   void notifyStateEntry();

   /** Notify observer that we left old state */
   void notifyStateExit();

   mutable SipConnectionStateContext m_rStateContext; ///< context containing state of sip connection. Needs to be locked when accessed.
   BaseSipConnectionState* m_pSipConnectionState; ///< pointer to state object handling commands and SipMessageEvents
   SipConnectionStateObserver* m_pStateObserver; ///< observer for state changes
   SipUserAgent& m_rSipUserAgent; ///< sip user agent
   CpMediaInterfaceProvider& m_rMediaInterfaceProvider; ///< provider of CpMediaInterface
   CpMessageQueueProvider& m_rMessageQueueProvider; ///< message queue provider
   XSipConnectionEventSink& m_rSipConnectionEventSink; ///< event sink (router) for various sip connection event types
   CpNatTraversalConfig m_natTraversalConfig; ///< NAT traversal configuration
};

#endif // SipConnectionStateMachine_h__
