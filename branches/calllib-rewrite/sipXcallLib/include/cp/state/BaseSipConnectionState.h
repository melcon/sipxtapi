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
#include <cp/state/ISipConnectionState.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class XSipConnectionContext;
class SipUserAgent;
class CpMediaInterfaceProvider;
class XSipConnectionEventSink;
class SipConnectionStateTransition;

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

   BaseSipConnectionState(XSipConnectionContext& rSipConnectionContext,
                          SipUserAgent& rSipUserAgent,
                          CpMediaInterfaceProvider* pMediaInterfaceProvider = NULL,
                          XSipConnectionEventSink* pSipConnectionEventSink = NULL);

   virtual ~BaseSipConnectionState();

   /* ============================ MANIPULATORS ============================== */

   /**
    * Default state entry handler.
    */
   virtual void handleStateEntry(StateEnum previousState);

   /**
    * Default state exit handler.
    */
   virtual void handleStateExit(StateEnum nextState);

   /**
   * Handles SipMessageEvent, which can be inbound sip request/response or error
   * sending status.
   * If instance cannot handle this event, it must pass it to parent as the last resort.
   *
   * @param rEvent Instance of SipMessageEvent that needs to be handled.
   * @return Instance of SipConnectionStateTransition if a transition should occur. NULL if no transition should occur.
   */
   virtual SipConnectionStateTransition* handleSipMessageEvent(const SipMessageEvent& rEvent);

   /**
    * Constructs transition from current state into given destination state.
    */
   SipConnectionStateTransition* getTransition(ISipConnectionState::StateEnum nextState);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   XSipConnectionContext& m_rSipConnectionContext; ///< context containing state of sip connection. Needs to be locked when accessed.
   SipUserAgent& m_rSipUserAgent; // for sending sip messages
   CpMediaInterfaceProvider* m_pMediaInterfaceProvider; ///< media interface provider
   XSipConnectionEventSink* m_pSipConnectionEventSink; ///< event sink (router) for various sip connection event types

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // BaseSipConnectionState_h__
