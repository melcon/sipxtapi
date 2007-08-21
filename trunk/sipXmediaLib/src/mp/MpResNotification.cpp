//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "mp/MpResNotification.h"
#include "mp/MpResource.h"

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

MpResNotification::MpResNotification(MpResource* pResource, MpResNotificationType type)
: m_bEnabled(true)
, m_type(type)
, m_pResource(pResource)
{
}

MpResNotification::~MpResNotification()
{

}

/* ============================ MANIPULATORS ============================== */

OsStatus MpResNotification::signal(const intptr_t eventData)
{
   if (m_bEnabled && m_pResource)
   {
      return m_pResource->notify(m_type, eventData);
   }
   else
   {
      return OS_FAILED;
   }
}

void MpResNotification::enable()
{
   m_bEnabled = true;
}

void MpResNotification::disable()
{
   m_bEnabled = false;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


