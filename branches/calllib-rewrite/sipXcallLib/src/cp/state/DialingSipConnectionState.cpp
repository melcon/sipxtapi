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
#include <cp/CpMediaInterfaceProvider.h>
#include <cp/state/DialingSipConnectionState.h>
#include <cp/state/UnknownSipConnectionState.h>
#include <cp/state/DisconnectedSipConnectionState.h>
#include <cp/state/RemoteOfferingSipConnectionState.h>
#include <cp/state/RemoteAlertingSipConnectionState.h>
#include <cp/state/RemoteQueuedSipConnectionState.h>
#include <cp/state/StateTransitionEventDispatcher.h>
#include <cp/state/GeneralTransitionMemory.h>

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
                                                     CpMessageQueueProvider& rMessageQueueProvider,
                                                     XSipConnectionEventSink& rSipConnectionEventSink,
                                                     const CpNatTraversalConfig& natTraversalConfig)
: BaseSipConnectionState(rStateContext, rSipUserAgent, rMediaInterfaceProvider, rMessageQueueProvider,
                         rSipConnectionEventSink, natTraversalConfig)
{

}

DialingSipConnectionState::DialingSipConnectionState(const BaseSipConnectionState& rhs)
: BaseSipConnectionState(rhs)
{

}

DialingSipConnectionState::~DialingSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void DialingSipConnectionState::handleStateEntry(StateEnum previousState, const StateTransitionMemory* pTransitionMemory)
{
   StateTransitionEventDispatcher eventDispatcher(m_rSipConnectionEventSink, pTransitionMemory);
   eventDispatcher.dispatchEvent(getCurrentState());

   OsSysLog::add(FAC_CP, PRI_DEBUG, "Entry dialing connection state from state: %d, sip call-id: %s\r\n",
      (int)previousState, getCallId().data());
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
   m_rStateContext.m_contactId = contactId;

   SipMessage sipInvite;
   Url fromField(fromAddress);
   int cseqNum = 0;

   UtlString contactUrl = buildContactUrl(fromField); // fromUrl without tag
   fromField.setFieldParameter("tag", localTag);
   secureUrl(fromField);
   sipInvite.setSecurityAttributes(m_rStateContext.m_pSecurity);
   getTransactionManager().startInitialInviteTransaction(cseqNum);

   sipInvite.setInviteData(fromField.toString(), toAddress,
      NULL, contactUrl, sipCallId,
      cseqNum, m_rStateContext.m_sessionReinviteSec);

   if (!locationHeader.isNull())
   {
      m_rStateContext.m_locationHeader = locationHeader;
      sipInvite.setLocationField(locationHeader);
   }

   initializeSipDialog(sipInvite);

   // check if some audio device is available
   if (!m_rMediaInterfaceProvider.getMediaInterface()->isAudioAvailable())
   {
      GeneralTransitionMemory memory(CP_CALLSTATE_CAUSE_RESOURCE_LIMIT);
      return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
   }

   // add SDP if negotiation mode is immediate, otherwise don't add it
   if (m_rStateContext.m_sdpNegotiation.getSdpOfferingMode() == CpSdpNegotiation::SDP_OFFERING_IMMEDIATE)
   {
      if (!startSdpNegotiation(sipInvite))
      {
         // SDP negotiation start failed
         // media connection creation failed
         GeneralTransitionMemory memory(CP_CALLSTATE_CAUSE_RESOURCE_LIMIT);
         return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
      }
   }

   // try to send sip message
   UtlBoolean sendSuccess = sendMessage(sipInvite);

   if (sendSuccess)
   {
      return getTransition(ISipConnectionState::CONNECTION_REMOTE_OFFERING, NULL);
   }
   else
   {
      GeneralTransitionMemory memory(CP_CALLSTATE_CAUSE_NETWORK);
      return getTransition(ISipConnectionState::CONNECTION_DISCONNECTED, &memory);
   }
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
         pDestination = new RemoteOfferingSipConnectionState(*this);
         break;
      case ISipConnectionState::CONNECTION_REMOTE_ALERTING:
         pDestination = new RemoteAlertingSipConnectionState(*this);
         break;
      case ISipConnectionState::CONNECTION_REMOTE_QUEUED:
         pDestination = new RemoteQueuedSipConnectionState(*this);
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
