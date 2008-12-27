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
#include <cp/CpSessionTimerProperties.h>

// DEFINES
#define MIN_SESSION_EXPIRES 90
#define INITIAL_SESSION_EXPIRES 3600

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

CpSessionTimerProperties::CpSessionTimerProperties()
: m_sessionExpires(INITIAL_SESSION_EXPIRES)
, m_initialSessionExpires(INITIAL_SESSION_EXPIRES)
, m_minSessionExpires(MIN_SESSION_EXPIRES)
{

}

CpSessionTimerProperties::~CpSessionTimerProperties()
{

}

/* ============================ MANIPULATORS ============================== */

void CpSessionTimerProperties::reset(UtlBoolean bFullReset)
{
   if (bFullReset)
   {
      m_sessionExpires = m_initialSessionExpires;
      m_minSessionExpires = MIN_SESSION_EXPIRES;
   }
   m_sRefresher = m_sInitialRefresher;
}

/* ============================ ACCESSORS ================================= */

CpSessionTimerProperties::RefresherType CpSessionTimerProperties::getRefresherType() const
{
   if (m_sRefresher.compareTo("uas") == 0)
   {
      return CpSessionTimerProperties::REFRESHER_UAS;
   }
   else if (m_sRefresher.compareTo("uac") == 0)
   {
      return CpSessionTimerProperties::REFRESHER_UAC;
   }
   else return CpSessionTimerProperties::REFRESH_DISABLED;
}

void CpSessionTimerProperties::setMinSessionExpires(int val)
{
   if (val < MIN_SESSION_EXPIRES)
   {
      // minimum value by RFC4028
      val = MIN_SESSION_EXPIRES;
   }
   m_minSessionExpires = val;
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
