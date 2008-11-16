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

OsStatus BaseSipConnectionState::connect(const UtlString& sipCallId,
                                         const UtlString& localTag,
                                         const UtlString& toAddress,
                                         const UtlString& fromAddress,
                                         const UtlString& locationHeader,
                                         CP_CONTACT_ID contactId)
{
   return OS_FAILED;
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

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
