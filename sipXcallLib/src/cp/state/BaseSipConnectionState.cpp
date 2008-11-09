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
#include <cp/state/BaseSipConnectionState.h>
#include <cp/state/SipConnectionStateTransition.h>

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

BaseSipConnectionState::BaseSipConnectionState(XSipConnectionContext& rSipConnectionContext,
                                               SipUserAgent& rSipUserAgent,
                                               CpMediaInterfaceProvider* pMediaInterfaceProvider,
                                               XSipConnectionEventSink* pSipConnectionEventSink)
: m_rSipConnectionContext(rSipConnectionContext)
, m_rSipUserAgent(rSipUserAgent)
, m_pMediaInterfaceProvider(pMediaInterfaceProvider)
, m_pSipConnectionEventSink(pSipConnectionEventSink)
{

}

BaseSipConnectionState::~BaseSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void BaseSipConnectionState::handleStateEntry(StateEnum previousState)
{
}

void BaseSipConnectionState::handleStateExit(StateEnum nextState)
{
}

SipConnectionStateTransition* BaseSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::getTransition(ISipConnectionState::StateEnum nextState)
{
   if (this->getCurrentState() != nextState)
   {
      // TODO: construct new state and pass it into transition
      SipConnectionStateTransition* pTransition = new SipConnectionStateTransition(this, NULL);
      return pTransition;
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
