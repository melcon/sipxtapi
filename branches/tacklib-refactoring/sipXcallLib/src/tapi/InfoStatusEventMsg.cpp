//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <tapi/InfoStatusEventMsg.h>

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

InfoStatusEventMsg::InfoStatusEventMsg() : OsMsg(INFOSTATUS_MSG, 0)
, m_event(INFOSTATUS_UNKNOWN)
, m_hInfo(0)
, m_eventPayload()
{

}

InfoStatusEventMsg::InfoStatusEventMsg(SIPX_INFOSTATUS_EVENT event,
                                       SIPX_INFO hInfo,
                                       const SipInfoStatusEvent& eventPayload) : OsMsg(INFOSTATUS_MSG, 0)
, m_event(event)
, m_hInfo(hInfo)
, m_eventPayload(eventPayload)
{

}

InfoStatusEventMsg::~InfoStatusEventMsg()
{

}

OsMsg* InfoStatusEventMsg::createCopy(void) const
{
   return new InfoStatusEventMsg(m_event, m_hInfo, m_eventPayload);
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
