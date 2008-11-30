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
#include <os/OsSysLog.h>
#include <cp/state/SipConnectionStateMachine.h>
#include <cp/state/IdleSipConnectionState.h>
#include <cp/state/SipConnectionStateObserver.h>
#include <cp/state/SipConnectionStateTransition.h>
#include <cp/state/DialingSipConnectionState.h>

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

SipConnectionStateMachine::SipConnectionStateMachine(SipUserAgent& rSipUserAgent,
                                                     CpMediaInterfaceProvider& rMediaInterfaceProvider,
                                                     XSipConnectionEventSink& rSipConnectionEventSink,
                                                     const CpNatTraversalConfig& natTraversalConfig)
: m_rStateContext()
, m_pSipConnectionState(NULL)
, m_rSipUserAgent(rSipUserAgent)
, m_rMediaInterfaceProvider(rMediaInterfaceProvider)
, m_rSipConnectionEventSink(rSipConnectionEventSink)
, m_natTraversalConfig(natTraversalConfig)
{
   // deleted in handleStateTransition if unsuccessful
   BaseSipConnectionState* pSipConnectionState = new IdleSipConnectionState(m_rStateContext, m_rSipUserAgent,
      m_rMediaInterfaceProvider, m_rSipConnectionEventSink, m_natTraversalConfig);
   SipConnectionStateTransition transition(m_pSipConnectionState, pSipConnectionState);

   handleStateTransition(transition);
}

SipConnectionStateMachine::~SipConnectionStateMachine()
{
   SipConnectionStateTransition transition(m_pSipConnectionState, NULL);
   handleStateTransition(transition);
}

/* ============================ MANIPULATORS ============================== */

void SipConnectionStateMachine::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->handleSipMessageEvent(rEvent));
   }
}

OsStatus SipConnectionStateMachine::connect(const UtlString& toAddress,
                                            const UtlString& fromAddress,
                                            const UtlString& locationHeader,
                                            CP_CONTACT_ID contactId)
{
   if (getCurrentState() == ISipConnectionState::CONNECTION_IDLE)
   {
      // switch to dialing
      // deleted in doHandleStateTransition if unsuccessful
      BaseSipConnectionState* pSipConnectionState = new DialingSipConnectionState(m_rStateContext, m_rSipUserAgent,
         m_rMediaInterfaceProvider, m_rSipConnectionEventSink, m_natTraversalConfig);
      SipConnectionStateTransition transition(m_pSipConnectionState, pSipConnectionState);
      handleStateTransition(transition);
   }

   // now let state handle request
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->connect(toAddress, fromAddress,
         locationHeader, contactId));
      return OS_SUCCESS;
   }

   return OS_FAILED;
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

XSipConnectionContext& SipConnectionStateMachine::getSipConnectionContext() const
{
   return m_rStateContext;
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void SipConnectionStateMachine::handleStateTransition(SipConnectionStateTransition* pStateTransition)
{
   if (pStateTransition)
   {
      handleStateTransition(*pStateTransition);
      delete pStateTransition;
      pStateTransition = NULL;
   }
}

void SipConnectionStateMachine::handleStateTransition(SipConnectionStateTransition& rStateTransition)
{
   if (rStateTransition.getSource() == m_pSipConnectionState)
   {
      // transition seems to be valid
      ISipConnectionState::StateEnum previousState = ISipConnectionState::CONNECTION_UNKNOWN;
      ISipConnectionState::StateEnum nextState = ISipConnectionState::CONNECTION_UNKNOWN;
      BaseSipConnectionState* m_pDestination = rStateTransition.getDestination();

      if (m_pDestination)
      {
         nextState = m_pDestination->getCurrentState();
      }
      if (m_pSipConnectionState)
      {
         previousState = m_pSipConnectionState->getCurrentState();
         m_pSipConnectionState->handleStateExit(nextState, rStateTransition.getMemory());
         notifyStateExit(); // also notify observer
         // delete old state
         delete m_pSipConnectionState;
         m_pSipConnectionState = NULL;
      }
      if (m_pDestination)
      {
         m_pSipConnectionState = m_pDestination;
         m_pSipConnectionState->handleStateEntry(previousState, rStateTransition.getMemory());
         notifyStateEntry(); // also notify observer
      }
   }
   else
   {
      OsSysLog::add(FAC_CP, PRI_ERR, "Invalid state transition in SipConnectionStateMachine, source state mismatch.");
      // delete destination state to avoid leaks
      BaseSipConnectionState* m_pDestination = rStateTransition.getDestination();
      if (m_pDestination)
      {
         delete m_pDestination;
         m_pDestination = NULL;
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
