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
#include <cp/state/DisconnectedSipConnectionState.h>

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

DisconnectedSipConnectionState::DisconnectedSipConnectionState(XSipConnectionContext& rSipConnectionContext,
                                                               SipUserAgent& rSipUserAgent,
                                                               CpMediaInterfaceProvider* pMediaInterfaceProvider)
: BaseSipConnectionState(rSipConnectionContext, rSipUserAgent, pMediaInterfaceProvider)
{

}

DisconnectedSipConnectionState::~DisconnectedSipConnectionState()
{

}

/* ============================ MANIPULATORS ============================== */

void DisconnectedSipConnectionState::handleStateEntry()
{
}

void DisconnectedSipConnectionState::handleStateExit()
{
}

ISipConnectionState* DisconnectedSipConnectionState::handleSipMessageEvent(const SipMessageEvent& rEvent)
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
