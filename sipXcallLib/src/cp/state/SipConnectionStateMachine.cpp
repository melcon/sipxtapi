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
#include <cp/state/SipConnectionStateMachine.h>
#include <cp/state/IdleSipConnectionState.h>
#include <cp/state/SipConnectionStateObserver.h>

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

SipConnectionStateMachine::SipConnectionStateMachine(XSipConnectionContext& rSipConnectionContext,
                                                     SipUserAgent& rSipUserAgent,
                                                     CpMediaInterfaceProvider* pMediaInterfaceProvider,
                                                     XSipConnectionEventSink* pSipConnectionEventSink)
: m_rSipConnectionContext(rSipConnectionContext)
, m_pSipConnectionState(NULL)
, m_rSipUserAgent(rSipUserAgent)
, m_pMediaInterfaceProvider(pMediaInterfaceProvider)
, m_pSipConnectionEventSink(pSipConnectionEventSink)
{
   setStateObject(new IdleSipConnectionState(m_rSipConnectionContext, m_rSipUserAgent,
      m_pMediaInterfaceProvider, m_pSipConnectionEventSink));
}

SipConnectionStateMachine::~SipConnectionStateMachine()
{
   setStateObject(NULL);
   m_pMediaInterfaceProvider = NULL;
}

/* ============================ MANIPULATORS ============================== */

void SipConnectionStateMachine::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   if (m_pSipConnectionState)
   {
      setStateObject(m_pSipConnectionState->handleSipMessageEvent(rEvent));
   }
}

/* ============================ ACCESSORS ================================= */

ISipConnectionState::StateEnum SipConnectionStateMachine::getCurrentState()
{
   if (m_pSipConnectionState)
   {
      return m_pSipConnectionState->getCurrentState();
   }
   else
   {
      return ISipConnectionState::CONNECTION_UNKNOWN;
   }
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void SipConnectionStateMachine::setStateObject(ISipConnectionState* pNewState)
{
   if (m_pSipConnectionState != pNewState)
   {
      // new state is different than old one
      if (m_pSipConnectionState)
      {
         m_pSipConnectionState->handleStateExit();
         notifyStateExit(); // also notify observer
         // delete old state
         delete m_pSipConnectionState;
         m_pSipConnectionState = NULL;
      }
      if (pNewState)
      {
         m_pSipConnectionState = pNewState;
         m_pSipConnectionState->handleStateEntry();
         notifyStateEntry(); // also notify observer
      }
   }
}

void SipConnectionStateMachine::notifyStateEntry()
{
   if (m_pStateObserver && m_pSipConnectionState)
   {
      m_pStateObserver->handleStateEntry(m_pSipConnectionState->getCurrentState());
   }
}

void SipConnectionStateMachine::notifyStateExit()
{
   if (m_pStateObserver && m_pSipConnectionState)
   {
      m_pStateObserver->handleStateExit(m_pSipConnectionState->getCurrentState());
   }
}

/* ============================ FUNCTIONS ================================= */
