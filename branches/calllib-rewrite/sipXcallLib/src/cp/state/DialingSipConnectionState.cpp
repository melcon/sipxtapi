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
#include <cp/state/DialingSipConnectionState.h>
#include <cp/state/FailedSipConnectionState.h>
#include <cp/state/UnknownSipConnectionState.h>
#include <cp/state/DisconnectedSipConnectionState.h>
#include <cp/state/RemoteOfferingSipConnectionState.h>
#include <cp/state/RemoteAlertingSipConnectionState.h>
#include <cp/state/RemoteQueuedSipConnectionState.h>

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

DialingSipConnectionState::DialingSipConnectionState(SipConnectionStateContext& rStateContext,
                                                     SipUserAgent& rSipUserAgent,
                                                     CpMediaInterfaceProvider& rMediaInterfaceProvider,
                                                     XSipConnectionEventSink& rSipConnectionEventSink)
: BaseSipConnectionState(rStateContext, rSipUserAgent, rMediaInterfaceProvider, rSipConnectionEventSink)
{

}

DialingSipConnectionState::~DialingSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void DialingSipConnectionState::handleStateEntry(StateEnum previousState, const StateTransitionMemory* pTransitionMemory)
{

}

void DialingSipConnectionState::handleStateExit(StateEnum nextState, const StateTransitionMemory* pTransitionMemory)
{

}

SipConnectionStateTransition* DialingSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   // handle event here

   // as a last resort, let parent handle event
   return BaseSipConnectionState::handleSipMessageEvent(rEvent);
}

SipConnectionStateTransition* DialingSipConnectionState::connect(const UtlString& sipCallId,
                                                                 const UtlString& localTag,
                                                                 const UtlString& toAddress,
                                                                 const UtlString& fromAddress,
                                                                 const UtlString& locationHeader,
                                                                 CP_CONTACT_ID contactId)
{
   return NULL;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

SipConnectionStateTransition* DialingSipConnectionState::getTransition(ISipConnectionState::StateEnum nextState,
                                                                       const StateTransitionMemory* pTansitionMemory) const
{
   if (this->getCurrentState() != nextState)
   {
      BaseSipConnectionState* pDestination = NULL;
      switch(nextState)
      {
      case ISipConnectionState::CONNECTION_REMOTE_OFFERING:
         pDestination = new RemoteOfferingSipConnectionState(m_rStateContext, m_rSipUserAgent,
            m_rMediaInterfaceProvider, m_rSipConnectionEventSink);
         break;
      case ISipConnectionState::CONNECTION_REMOTE_ALERTING:
         pDestination = new RemoteAlertingSipConnectionState(m_rStateContext, m_rSipUserAgent,
            m_rMediaInterfaceProvider, m_rSipConnectionEventSink);
         break;
      case ISipConnectionState::CONNECTION_REMOTE_QUEUED:
         pDestination = new RemoteQueuedSipConnectionState(m_rStateContext, m_rSipUserAgent,
            m_rMediaInterfaceProvider, m_rSipConnectionEventSink);
         break;
      case ISipConnectionState::CONNECTION_FAILED:
         pDestination = new FailedSipConnectionState(m_rStateContext, m_rSipUserAgent,
            m_rMediaInterfaceProvider, m_rSipConnectionEventSink);
         break;
      case ISipConnectionState::CONNECTION_DISCONNECTED:
         pDestination = new DisconnectedSipConnectionState(m_rStateContext, m_rSipUserAgent,
            m_rMediaInterfaceProvider, m_rSipConnectionEventSink);
         break;
      case ISipConnectionState::CONNECTION_UNKNOWN:
      default:
         pDestination = new UnknownSipConnectionState(m_rStateContext, m_rSipUserAgent,
            m_rMediaInterfaceProvider, m_rSipConnectionEventSink);
         break;
      }

      return getTransitionObject(pDestination, pTansitionMemory);
   }
   else
   {
      return NULL;
   }
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
