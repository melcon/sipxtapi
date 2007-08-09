//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
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
#include "tapi/SipXKeepaliveEventListener.h"
#include "tapi/sipXtapiEvents.h"
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

SipXKeepaliveEventListener::SipXKeepaliveEventListener( SIPX_INST pInst ) : OsNatKeepaliveListener()
{
   m_pInst = pInst;
}

/* ============================ MANIPULATORS ============================== */


void SipXKeepaliveEventListener::OnKeepaliveStart( const OsNatKeepaliveEvent& event )
{
   sipxFireKeepaliveEvent(m_pInst, KEEPALIVE_START, KEEPALIVE_CAUSE_NORMAL,
      (SIPX_KEEPALIVE_TYPE) event.type, 
      event.remoteAddress, event.remotePort,
      event.keepAliveSecs,
      event.mappedAddress, event.mappedPort);
}

void SipXKeepaliveEventListener::OnKeepaliveStop( const OsNatKeepaliveEvent& event )
{
   sipxFireKeepaliveEvent(m_pInst, KEEPALIVE_STOP, KEEPALIVE_CAUSE_NORMAL,
      (SIPX_KEEPALIVE_TYPE) event.type, 
      event.remoteAddress, event.remotePort,
      event.keepAliveSecs,
      event.mappedAddress, event.mappedPort);
}

void SipXKeepaliveEventListener::OnKeepaliveFeedback( const OsNatKeepaliveEvent& event )
{
   sipxFireKeepaliveEvent(m_pInst, KEEPALIVE_FEEDBACK, KEEPALIVE_CAUSE_NORMAL,
      (SIPX_KEEPALIVE_TYPE) event.type, 
      event.remoteAddress, event.remotePort,
      event.keepAliveSecs,
      event.mappedAddress, event.mappedPort);
}

void SipXKeepaliveEventListener::OnKeepaliveFailure( const OsNatKeepaliveEvent& event )
{
   sipxFireKeepaliveEvent(m_pInst, KEEPALIVE_FAILURE, KEEPALIVE_CAUSE_NORMAL,
      (SIPX_KEEPALIVE_TYPE) event.type, 
      event.remoteAddress, event.remotePort,
      event.keepAliveSecs,
      event.mappedAddress, event.mappedPort);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


