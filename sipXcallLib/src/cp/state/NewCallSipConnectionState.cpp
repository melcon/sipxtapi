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
#include <cp/state/NewCallSipConnectionState.h>
#include <cp/state/FailedSipConnectionState.h>
#include <cp/state/UnknownSipConnectionState.h>
#include <cp/state/DisconnectedSipConnectionState.h>
#include <cp/state/OfferingSipConnectionState.h>

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

NewCallSipConnectionState::NewCallSipConnectionState(SipConnectionStateContext& rStateContext,
                                                     SipUserAgent& rSipUserAgent,
                                                     CpMediaInterfaceProvider& rMediaInterfaceProvider,
                                                     XSipConnectionEventSink& rSipConnectionEventSink)
: BaseSipConnectionState(rStateContext, rSipUserAgent, rMediaInterfaceProvider, rSipConnectionEventSink)
{

}

NewCallSipConnectionState::~NewCallSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void NewCallSipConnectionState::handleStateEntry(StateEnum previousState, const StateTransitionMemory* pTransitionMemory)
{

}

void NewCallSipConnectionState::handleStateExit(StateEnum nextState, const StateTransitionMemory* pTransitionMemory)
{

}

SipConnectionStateTransition* NewCallSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   // handle event here

   // as a last resort, let parent handle event
   return BaseSipConnectionState::handleSipMessageEvent(rEvent);
}

SipConnectionStateTransition* NewCallSipConnectionState::getTransition(ISipConnectionState::StateEnum nextState,
                                                                       const StateTransitionMemory* pTansitionMemory) const
{
   if (this->getCurrentState() != nextState)
   {
      BaseSipConnectionState* pDestination = NULL;
      switch(nextState)
      {
      case ISipConnectionState::CONNECTION_OFFERING:
         pDestination = new OfferingSipConnectionState(m_rStateContext, m_rSipUserAgent,
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

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
