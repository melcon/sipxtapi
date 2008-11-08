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
#include <cp/state/ISipConnectionState.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class ISipConnectionState;
class SipMessageEvent;
class XSipConnectionContext;
class SipConnectionStateObserver;
class SipUserAgent;
class CpMediaInterfaceProvider;

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
   
   SipConnectionStateMachine(XSipConnectionContext& rSipConnectionContext,
                             SipUserAgent& rSipUserAgent,
                             CpMediaInterfaceProvider* pMediaInterfaceProvider = NULL);

   virtual ~SipConnectionStateMachine();

   /* ============================ MANIPULATORS ============================== */

   /**
    * Handles SipMessageEvent, which can be inbound SipMessage or notification about
    * send failure.
    */
   void handleSipMessageEvent(const SipMessageEvent& rEvent);

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

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   SipConnectionStateMachine(const SipConnectionStateMachine& rhs);

   SipConnectionStateMachine& operator=(const SipConnectionStateMachine& rhs);

   void setStateObject(ISipConnectionState* pNewState);

   /** Notify observer that we entered new state */
   void notifyStateEntry();

   /** Notify observer that we left old state */
   void notifyStateExit();

   XSipConnectionContext& m_rSipConnectionContext; ///< context containing state of sip connection. Needs to be locked when accessed.
   ISipConnectionState* m_pSipConnectionState; ///< pointer to state object handling commands and SipMessageEvents
   SipConnectionStateObserver* m_pStateObserver; ///< observer for state changes
   SipUserAgent& m_rSipUserAgent; ///< sip user agent
   CpMediaInterfaceProvider* m_pMediaInterfaceProvider; ///< provider of CpMediaInterface
};

#endif // SipConnectionStateMachine_h__
