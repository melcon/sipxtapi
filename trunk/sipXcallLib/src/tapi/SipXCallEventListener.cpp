//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/SipXCallEventListener.h"
#include "tapi/SipXEvents.h"

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

SipXCallEventListener::SipXCallEventListener( SIPX_INST pInst )
: m_pInst(pInst)
{

}

SipXCallEventListener::~SipXCallEventListener()
{

}

/* ============================ MANIPULATORS ============================== */

void SipXCallEventListener::OnNewCall( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_NEWCALL,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);
}

void SipXCallEventListener::OnDialTone( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_DIALTONE,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);
}

void SipXCallEventListener::OnRemoteOffering( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_REMOTE_OFFERING,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);
}

void SipXCallEventListener::OnRemoteAlerting( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_REMOTE_ALERTING,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);
}

void SipXCallEventListener::OnConnected( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_CONNECTED,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);
}

void SipXCallEventListener::OnBridged( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_BRIDGED,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);
}

void SipXCallEventListener::OnHeld( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_HELD,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);
}

void SipXCallEventListener::OnRemoteHeld( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_REMOTE_HELD,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);

}

void SipXCallEventListener::OnDisconnected( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_DISCONNECTED,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);

}

void SipXCallEventListener::OnOffering( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_OFFERING,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);

}

void SipXCallEventListener::OnAlerting( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_ALERTING,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);

}

void SipXCallEventListener::OnDestroyed( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_DESTROYED,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);

}

void SipXCallEventListener::OnTransferEvent( const CpCallStateEvent& event )
{
   sipxFireCallEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_Session,
                     event.m_sRemoteAddress,
                     CALLSTATE_TRANSFER_EVENT,
                     event.m_cause,
                     event.m_pEventData,
                     event.m_sipResponseCode,
                     event.m_sResponseText);

}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

