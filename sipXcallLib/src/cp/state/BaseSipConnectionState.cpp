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
#include <os/OsWriteLock.h>
#include <net/SipMessage.h>
#include <net/SipUserAgent.h>
#include <cp/XSipConnectionContext.h>
#include <cp/state/BaseSipConnectionState.h>
#include <cp/state/SipConnectionStateTransition.h>
#include <cp/state/StateTransitionMemory.h>

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

BaseSipConnectionState::BaseSipConnectionState(SipConnectionStateContext& rStateContext,
                                               SipUserAgent& rSipUserAgent,
                                               CpMediaInterfaceProvider& rMediaInterfaceProvider,
                                               XSipConnectionEventSink& rSipConnectionEventSink,
                                               const CpNatTraversalConfig& natTraversalConfig)
: m_rStateContext(rStateContext)
, m_rSipUserAgent(rSipUserAgent)
, m_rMediaInterfaceProvider(rMediaInterfaceProvider)
, m_rSipConnectionEventSink(rSipConnectionEventSink)
, m_natTraversalConfig(natTraversalConfig)
{

}

BaseSipConnectionState::BaseSipConnectionState(const BaseSipConnectionState& rhs)
: m_rStateContext(rhs.m_rStateContext)
, m_rSipUserAgent(rhs.m_rSipUserAgent)
, m_rMediaInterfaceProvider(rhs.m_rMediaInterfaceProvider)
, m_rSipConnectionEventSink(rhs.m_rSipConnectionEventSink)
, m_natTraversalConfig(rhs.m_natTraversalConfig)
{

}

BaseSipConnectionState::~BaseSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void BaseSipConnectionState::handleStateEntry(StateEnum previousState, const StateTransitionMemory* pTransitionMemory)
{
}

void BaseSipConnectionState::handleStateExit(StateEnum nextState, const StateTransitionMemory* pTransitionMemory)
{
}

SipConnectionStateTransition* BaseSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   // TODO: Implement
   return NULL;
}

SipConnectionStateTransition* BaseSipConnectionState::connect(const UtlString& toAddress,
                                                              const UtlString& fromAddress,
                                                              const UtlString& locationHeader,
                                                              CP_CONTACT_ID contactId)
{
   return NULL;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

SipConnectionStateTransition* BaseSipConnectionState::getTransitionObject(BaseSipConnectionState* pDestination,
                                                                          const StateTransitionMemory* pTansitionMemory) const
{
   if (this != pDestination)
   {
      StateTransitionMemory* pTansitionMemoryCopy = NULL;
      if (pTansitionMemory)
      {
         pTansitionMemoryCopy = new StateTransitionMemory(*pTansitionMemory);
      }
      // construct transition from this to destination state, containing copy of state transition memory object
      SipConnectionStateTransition* pTransition = new SipConnectionStateTransition(this, pDestination,
         pTansitionMemoryCopy);
      return pTransition;
   }
   else
   {
      return NULL;
   }
}

void BaseSipConnectionState::updateSipDialog(const SipMessage& sipMessage)
{
   OsWriteLock lock(m_rStateContext);
   m_rStateContext.m_sipDialog.updateDialog(sipMessage);
}

UtlBoolean BaseSipConnectionState::sendMessage(SipMessage& sipMessage)
{
   updateSipDialog(sipMessage);
   return m_rSipUserAgent.send(sipMessage);
}

UtlBoolean BaseSipConnectionState::isMethodAllowed(const UtlString& sMethod)
{
   if (m_rStateContext.m_allowedRemote.index(sMethod) >=0 ||
       m_rStateContext.m_implicitAllowedRemote.index(sMethod) >= 0)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

UtlBoolean BaseSipConnectionState::isInviteTransactionActive() const
{
   return m_rStateContext.m_inviteTransactionState == SipConnectionStateContext::INVITE_ACTIVE;
}

void BaseSipConnectionState::startInviteTransaction()
{
   m_rStateContext.m_inviteTransactionState == SipConnectionStateContext::INVITE_ACTIVE;
}

void BaseSipConnectionState::stopInviteTransaction()
{
   m_rStateContext.m_inviteTransactionState == SipConnectionStateContext::INVITE_INACTIVE;
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
