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
class XSipConnectionEventSink;
class SipConnectionStateTransition;

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
   
   SipConnectionStateMachine(SipUserAgent& rSipUserAgent,
                             CpMediaInterfaceProvider& rMediaInterfaceProvider,
                             XSipConnectionEventSink& rSipConnectionEventSink);

   virtual ~SipConnectionStateMachine();

   /* ============================ MANIPULATORS ============================== */

   /**
    * Handles SipMessageEvent, which can be inbound SipMessage or notification about
    * send failure.
    */
   void handleSipMessageEvent(const SipMessageEvent& rEvent);

   /** Connects call to given address. Uses supplied sip call-id. */
   OsStatus connect(const UtlString& toAddress,
                    const UtlString& fromAddress,
                    const UtlString& locationHeader,
                    CP_CONTACT_ID contactId);

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

   /** Gets public Sip connection context. */
   XSipConnectionContext& getSipConnectionContext() const;

   /* ============================ INQUIRY =================================== */

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
   XSipConnectionEventSink& m_rSipConnectionEventSink; ///< event sink (router) for various sip connection event types
};

#endif // SipConnectionStateMachine_h__
