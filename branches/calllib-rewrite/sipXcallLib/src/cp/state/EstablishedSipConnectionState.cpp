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
#include <net/SipMessage.h>
#include <cp/state/EstablishedSipConnectionState.h>
#include <cp/state/UnknownSipConnectionState.h>
#include <cp/state/DisconnectedSipConnectionState.h>
#include <cp/state/StateTransitionEventDispatcher.h>

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

EstablishedSipConnectionState::EstablishedSipConnectionState(SipConnectionStateContext& rStateContext,
                                                             SipUserAgent& rSipUserAgent,
                                                             CpMediaInterfaceProvider& rMediaInterfaceProvider,
                                                             CpMessageQueueProvider& rMessageQueueProvider,
                                                             XSipConnectionEventSink& rSipConnectionEventSink,
                                                             const CpNatTraversalConfig& natTraversalConfig)
: BaseSipConnectionState(rStateContext, rSipUserAgent, rMediaInterfaceProvider, rMessageQueueProvider,
                         rSipConnectionEventSink, natTraversalConfig)
{

}

EstablishedSipConnectionState::EstablishedSipConnectionState(const BaseSipConnectionState& rhs)
: BaseSipConnectionState(rhs)
{

}

EstablishedSipConnectionState::~EstablishedSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void EstablishedSipConnectionState::handleStateEntry(StateEnum previousState, const StateTransitionMemory* pTransitionMemory)
{
   StateTransitionEventDispatcher eventDispatcher(m_rSipConnectionEventSink, pTransitionMemory);
   eventDispatcher.dispatchEvent(getCurrentState());

   OsSysLog::add(FAC_CP, PRI_DEBUG, "Entry established connection state from state: %d, sip call-id: %s\r\n",
      (int)previousState, getCallId().data());
}

void EstablishedSipConnectionState::handleStateExit(StateEnum nextState, const StateTransitionMemory* pTransitionMemory)
{

}

SipConnectionStateTransition* EstablishedSipConnectionState::dropConnection(OsStatus& result)
{
   if (!isLocalInitiatedDialog())
   {
      // inbound established call
      if (m_rStateContext.m_bAckReceived)
      {
         // we may send BYE
         return doByeConnection(result);
      }
      else
      {
         // we may not send BYE, we must wait
         startDelayedByeTimer(); // we will try BYE again later
      }
   }
   else
   {
      // outbound established call, we may use BYE
      return doByeConnection(result);
   }
}

SipConnectionStateTransition* EstablishedSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   // handle event here

   // as a last resort, let parent handle event
   return BaseSipConnectionState::handleSipMessageEvent(rEvent);
}

SipConnectionStateTransition* EstablishedSipConnectionState::processInviteResponse(const SipMessage& sipMessage)
{
   // first let parent handle response
   SipConnectionStateTransition* pTransition = BaseSipConnectionState::processInviteResponse(sipMessage);
   if (pTransition)
   {
      return pTransition;
   }

   int responseCode = sipMessage.getResponseStatusCode();
   UtlString responseText;
   sipMessage.getResponseStatusText(&responseText);

   switch (responseCode)
   {
   case SIP_OK_CODE:
   case SIP_ACCEPTED_CODE:
      {
         // send ACK to retransmitted 200 OK
         handle2xxResponse(sipMessage);
         break;
      }
   default:
      ;
   }

   // no transition
   return NULL;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

SipConnectionStateTransition* EstablishedSipConnectionState::getTransition(ISipConnectionState::StateEnum nextState,
                                                                           const StateTransitionMemory* pTansitionMemory) const
{
   if (this->getCurrentState() != nextState)
   {
      BaseSipConnectionState* pDestination = NULL;
      switch(nextState)
      {
      case ISipConnectionState::CONNECTION_ESTABLISHED:
         pDestination = new EstablishedSipConnectionState(*this);
         break;
      case ISipConnectionState::CONNECTION_DISCONNECTED:
         pDestination = new DisconnectedSipConnectionState(*this);
         break;
      case ISipConnectionState::CONNECTION_UNKNOWN:
      default:
         pDestination = new UnknownSipConnectionState(*this);
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
