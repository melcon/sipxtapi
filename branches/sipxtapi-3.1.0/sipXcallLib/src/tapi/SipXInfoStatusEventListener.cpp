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
#include "tapi/SipXInfoStatusEventListener.h"
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

SipXInfoStatusEventListener::SipXInfoStatusEventListener( SIPX_INST pInst )
      : m_pInst(pInst)
{

}

SipXInfoStatusEventListener::~SipXInfoStatusEventListener()
{

}

/* ============================ MANIPULATORS ============================== */

void SipXInfoStatusEventListener::OnResponse( const SipInfoStatusEvent& event )
{
   sipxFireInfoStatusEvent(m_pInst, 0, event.m_Status, event.m_iResponseCode, event.m_sResponseText, INFOSTATUS_RESPONSE);
}

void SipXInfoStatusEventListener::OnNetworkError( const SipInfoStatusEvent& event )
{
   sipxFireInfoStatusEvent(m_pInst, 0, event.m_Status, event.m_iResponseCode, event.m_sResponseText, INFOSTATUS_NETWORK_ERROR);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

