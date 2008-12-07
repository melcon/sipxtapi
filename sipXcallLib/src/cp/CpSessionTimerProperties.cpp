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
: m_sessionExpires(120)
, m_minSessionExpires(90)
{

}

CpSessionTimerProperties::~CpSessionTimerProperties()
{

}

/* ============================ MANIPULATORS ============================== */

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
   if (val < 90)
   {
      // minimum value by RFC4028
      val = 90;
   }
   m_minSessionExpires = val;
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
