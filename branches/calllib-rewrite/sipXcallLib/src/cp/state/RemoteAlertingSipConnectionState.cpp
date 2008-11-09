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
#include <cp/state/RemoteAlertingSipConnectionState.h>

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

RemoteAlertingSipConnectionState::RemoteAlertingSipConnectionState(XSipConnectionContext& rSipConnectionContext,
                                                                   SipUserAgent& rSipUserAgent,
                                                                   CpMediaInterfaceProvider* pMediaInterfaceProvider,
                                                                   XSipConnectionEventSink* pSipConnectionEventSink)
: BaseSipConnectionState(rSipConnectionContext, rSipUserAgent, pMediaInterfaceProvider, pSipConnectionEventSink)
{

}

RemoteAlertingSipConnectionState::~RemoteAlertingSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void RemoteAlertingSipConnectionState::handleStateEntry()
{

}

void RemoteAlertingSipConnectionState::handleStateExit()
{

}

ISipConnectionState* RemoteAlertingSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
{
   // handle event here

   // as a last resort, let parent handle event
   return BaseSipConnectionState::handleSipMessageEvent(rEvent);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
