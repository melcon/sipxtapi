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
#include <os/OsWriteLock.h>
#include <net/SipMessageEvent.h>
#include <cp/state/SipConnectionStateMachine.h>
#include <cp/state/IdleSipConnectionState.h>
#include <cp/state/SipConnectionStateObserver.h>
#include <cp/state/SipConnectionStateTransition.h>
#include <cp/state/DialingSipConnectionState.h>
#include <cp/state/NewCallSipConnectionState.h>

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
                                                     CpMessageQueueProvider& rMessageQueueProvider,
                                                     XSipConnectionEventSink& rSipConnectionEventSink,
                                                     const CpNatTraversalConfig& natTraversalConfig)
: m_rStateContext()
, m_pSipConnectionState(NULL)
, m_pStateObserver(NULL)
, m_rSipUserAgent(rSipUserAgent)
, m_rMediaInterfaceProvider(rMediaInterfaceProvider)
, m_rMessageQueueProvider(rMessageQueueProvider)
, m_rSipConnectionEventSink(rSipConnectionEventSink)
, m_natTraversalConfig(natTraversalConfig)
{
   m_rStateContext.m_sdpNegotiation.setSecurity(m_rStateContext.m_pSecurity); // wire security into sdp negotiation

   // deleted in handleStateTransition if unsuccessful
   BaseSipConnectionState* pSipConnectionState = new IdleSipConnectionState(m_rStateContext, m_rSipUserAgent,
      m_rMediaInterfaceProvider, m_rMessageQueueProvider, m_rSipConnectionEventSink, m_natTraversalConfig);
   SipConnectionStateTransition transition(m_pSipConnectionState, pSipConnectionState);

   handleStateTransition(transition);
}

SipConnectionStateMachine::~SipConnectionStateMachine()
{
   SipConnectionStateTransition transition(m_pSipConnectionState, NULL);
   handleStateTransition(transition);
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean SipConnectionStateMachine::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   if (getCurrentState() == ISipConnectionState::CONNECTION_IDLE)
   {
      const SipMessage* pSipMessage = rEvent.getMessage();
      if (pSipMessage && pSipMessage->isInviteRequest())
      {
         // we must switch state to newcall
         SipDialog sipDialog(pSipMessage); // construct new sip dialog
         {
            OsWriteLock lock(m_rStateContext);
            m_rStateContext.m_sipDialog = sipDialog; // save sip dialog
         }
         // switch to newcall
         // deleted in doHandleStateTransition if unsuccessful
         BaseSipConnectionState* pSipConnectionState = new NewCallSipConnectionState(m_rStateContext, m_rSipUserAgent,
            m_rMediaInterfaceProvider, m_rMessageQueueProvider, m_rSipConnectionEventSink, m_natTraversalConfig);
         SipConnectionStateTransition transition(m_pSipConnectionState, pSipConnectionState);
         handleStateTransition(transition);
      }
   }

   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->handleSipMessageEvent(rEvent));
      return TRUE;
   }

   return FALSE;
}

OsStatus SipConnectionStateMachine::connect(const UtlString& sipCallId,
                                            const UtlString& localTag,
                                            const UtlString& toAddress,
                                            const UtlString& fromAddress,
                                            const UtlString& locationHeader,
                                            CP_CONTACT_ID contactId)
{
   OsStatus result = OS_FAILED;

   if (getCurrentState() == ISipConnectionState::CONNECTION_IDLE)
   {
      // switch to dialing
      // deleted in doHandleStateTransition if unsuccessful
      BaseSipConnectionState* pSipConnectionState = new DialingSipConnectionState(m_rStateContext, m_rSipUserAgent,
         m_rMediaInterfaceProvider, m_rMessageQueueProvider, m_rSipConnectionEventSink, m_natTraversalConfig);
      SipConnectionStateTransition transition(m_pSipConnectionState, pSipConnectionState);
      handleStateTransition(transition);
   }

   // now let state handle request
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->connect(result, sipCallId, localTag, toAddress, fromAddress,
         locationHeader, contactId));
   }

   return result;
}

OsStatus SipConnectionStateMachine::acceptConnection(const UtlString& locationHeader, CP_CONTACT_ID contactId)
{
   OsStatus result = OS_FAILED;

   // now let state handle request
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->acceptConnection(result, locationHeader, contactId));
   }

   return result;
}

OsStatus SipConnectionStateMachine::rejectConnection()
{
   OsStatus result = OS_FAILED;

   // now let state handle request
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->rejectConnection(result));
   }

   return result;
}

OsStatus SipConnectionStateMachine::redirectConnection(const UtlString& sRedirectSipUrl)
{
   OsStatus result = OS_FAILED;

   // now let state handle request
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->redirectConnection(result, sRedirectSipUrl));
   }

   return result;
}

OsStatus SipConnectionStateMachine::answerConnection()
{
   OsStatus result = OS_FAILED;

   // now let state handle request
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->answerConnection(result));
   }

   return result;
}

OsStatus SipConnectionStateMachine::dropConnection()
{
   OsStatus result = OS_FAILED;

   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->dropConnection(result));
   }

   return result;
}

OsStatus SipConnectionStateMachine::holdConnection()
{
   OsStatus result = OS_FAILED;

   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->holdConnection(result));
   }

   return result;
}

OsStatus SipConnectionStateMachine::unholdConnection()
{
   OsStatus result = OS_FAILED;

   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->unholdConnection(result));
   }

   return result;
}

OsStatus SipConnectionStateMachine::renegotiateCodecsConnection()
{
   OsStatus result = OS_FAILED;

   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->renegotiateCodecsConnection(result));
   }

   return result;
}

OsStatus SipConnectionStateMachine::sendInfo(const UtlString& sContentType,
                                             const char* pContent,
                                             const size_t nContentLength,
                                             void* pCookie)
{
   OsStatus result = OS_FAILED;

   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->sendInfo(result, sContentType, pContent, nContentLength, pCookie));
   }

   return result;
}

UtlBoolean SipConnectionStateMachine::handleTimerMessage(const ScTimerMsg& timerMsg)
{
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->handleTimerMessage(timerMsg));
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

UtlBoolean SipConnectionStateMachine::handleCommandMessage(const ScCommandMsg& rMsg)
{
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->handleCommandMessage(rMsg));
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

UtlBoolean SipConnectionStateMachine::handleNotificationMessage(const ScNotificationMsg& rMsg)
{
   if (m_pSipConnectionState)
   {
      handleStateTransition(m_pSipConnectionState->handleNotificationMessage(rMsg));
      return TRUE;
   }
   else
   {
      return FALSE;
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

XSipConnectionContext& SipConnectionStateMachine::getSipConnectionContext() const
{
   return m_rStateContext;
}

/* ============================ INQUIRY =================================== */

SipConnectionStateContext::MediaSessionState SipConnectionStateMachine::getMediaSessionState() const
{
   return m_rStateContext.m_mediaSessionState;
}

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
