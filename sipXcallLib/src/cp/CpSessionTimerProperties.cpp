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
#define INITIAL_SESSION_EXPIRES 300
#define REFRESHER_UAS "uas"
#define REFRESHER_UAC "uac"

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

void CpSessionTimerProperties::setRefresher(UtlString refresher, UtlBoolean bIsOutboundTransaction)
{
   if (bIsOutboundTransaction)
   {
      if (refresher.compareTo(REFRESHER_UAC))
      {
         m_sRefresher = CP_SESSION_REFRESH_LOCAL;
      }
      else if (refresher.compareTo(REFRESHER_UAS))
      {
         m_sRefresher = CP_SESSION_REFRESH_REMOTE;
      }
      else m_sRefresher = CP_SESSION_REFRESH_AUTO;
   }
   else
   {
      if (refresher.compareTo(REFRESHER_UAC))
      {
         m_sRefresher = CP_SESSION_REFRESH_REMOTE;
      }
      else if (refresher.compareTo(REFRESHER_UAS))
      {
         m_sRefresher = CP_SESSION_REFRESH_LOCAL;
      }
      else m_sRefresher = CP_SESSION_REFRESH_AUTO;
   }
}

UtlString CpSessionTimerProperties::getRefresher(UtlBoolean bIsOutboundTransaction) const
{
   if (bIsOutboundTransaction)
   {
      switch (m_sRefresher)
      {
      case CP_SESSION_REFRESH_LOCAL:
         return REFRESHER_UAC;
      case CP_SESSION_REFRESH_REMOTE:
         return REFRESHER_UAS;
      default:
         return NULL;
      }
   }
   else
   {
      switch (m_sRefresher)
      {
      case CP_SESSION_REFRESH_LOCAL:
         return REFRESHER_UAS;
      case CP_SESSION_REFRESH_REMOTE:
         return REFRESHER_UAC;
      default:
         return NULL;
      }
   }
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
