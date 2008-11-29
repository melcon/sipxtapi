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
#include <cp/state/FailedSipConnectionState.h>
#include <cp/state/UnknownSipConnectionState.h>

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

FailedSipConnectionState::FailedSipConnectionState(SipConnectionStateContext& rStateContext,
                                                   SipUserAgent& rSipUserAgent,
                                                   CpMediaInterfaceProvider& rMediaInterfaceProvider,
                                                   XSipConnectionEventSink& rSipConnectionEventSink)
: BaseSipConnectionState(rStateContext, rSipUserAgent, rMediaInterfaceProvider, rSipConnectionEventSink)
{

}

FailedSipConnectionState::~FailedSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void FailedSipConnectionState::handleStateEntry(StateEnum previousState, const StateTransitionMemory* pTransitionMemory)
{

}

void FailedSipConnectionState::handleStateExit(StateEnum nextState, const StateTransitionMemory* pTransitionMemory)
{

}

SipConnectionStateTransition* FailedSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   // handle event here

   // as a last resort, let parent handle event
   return BaseSipConnectionState::handleSipMessageEvent(rEvent);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

SipConnectionStateTransition* FailedSipConnectionState::getTransition(ISipConnectionState::StateEnum nextState,
                                                                      const StateTransitionMemory* pTansitionMemory) const
{
   if (this->getCurrentState() != nextState)
   {
      BaseSipConnectionState* pDestination = NULL;
      switch(nextState)
      {
      case ISipConnectionState::CONNECTION_FAILED:
         pDestination = new FailedSipConnectionState(m_rStateContext, m_rSipUserAgent,
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
