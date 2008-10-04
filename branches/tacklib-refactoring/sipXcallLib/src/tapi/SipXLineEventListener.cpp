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
#include "tapi/SipXLineEventListener.h"
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

SipXLineEventListener::SipXLineEventListener( SIPX_INST pInst )
   : m_pInst(pInst)
{

}

SipXLineEventListener::~SipXLineEventListener()
{

}

/* ============================ MANIPULATORS ============================== */

void SipXLineEventListener::OnLineRegistering( const SipLineStateEvent& event )
{
   sipxFireLineEvent(m_pInst, event.m_sLineId, LINESTATE_REGISTERING, (SIPX_LINESTATE_CAUSE)event.m_Cause, event.m_responseCode, event.m_sResponseText);
}

void SipXLineEventListener::OnLineRegistered( const SipLineStateEvent& event )
{
   sipxFireLineEvent(m_pInst, event.m_sLineId, LINESTATE_REGISTERED, (SIPX_LINESTATE_CAUSE)event.m_Cause, event.m_responseCode, event.m_sResponseText);
}

void SipXLineEventListener::OnLineUnregistering( const SipLineStateEvent& event )
{
   sipxFireLineEvent(m_pInst, event.m_sLineId, LINESTATE_UNREGISTERING, (SIPX_LINESTATE_CAUSE)event.m_Cause, event.m_responseCode, event.m_sResponseText);
}

void SipXLineEventListener::OnLineUnregistered( const SipLineStateEvent& event )
{
   sipxFireLineEvent(m_pInst, event.m_sLineId, LINESTATE_UNREGISTERED, (SIPX_LINESTATE_CAUSE)event.m_Cause, event.m_responseCode, event.m_sResponseText);
}

void SipXLineEventListener::OnLineRegisterFailed( const SipLineStateEvent& event )
{
   sipxFireLineEvent(m_pInst, event.m_sLineId, LINESTATE_REGISTER_FAILED, (SIPX_LINESTATE_CAUSE)event.m_Cause, event.m_responseCode, event.m_sResponseText);
}

void SipXLineEventListener::OnLineUnregisterFailed( const SipLineStateEvent& event )
{
   sipxFireLineEvent(m_pInst, event.m_sLineId, LINESTATE_UNREGISTER_FAILED, (SIPX_LINESTATE_CAUSE)event.m_Cause, event.m_responseCode, event.m_sResponseText);
}

void SipXLineEventListener::OnLineProvisioned( const SipLineStateEvent& event )
{
   sipxFireLineEvent(m_pInst, event.m_sLineId, LINESTATE_PROVISIONED, (SIPX_LINESTATE_CAUSE)event.m_Cause, event.m_responseCode, event.m_sResponseText);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

