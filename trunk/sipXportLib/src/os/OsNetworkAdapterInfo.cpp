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
#include <os/OsNetworkAdapterInfo.h>

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

OsNetworkAdapterInfo::OsNetworkAdapterInfo(unsigned long index,
                                           const UtlString& ipAddress,
                                           const UtlString& name,
                                           const UtlString& description)
: m_index(index)
, m_ipAddress(ipAddress)
, m_name(name)
, m_description(description)
{

}

OsNetworkAdapterInfo::~OsNetworkAdapterInfo()
{

}

OsNetworkAdapterInfo::OsNetworkAdapterInfo(const OsNetworkAdapterInfo& rhs)
: m_index(rhs.m_index)
, m_ipAddress(rhs.m_ipAddress)
, m_name(rhs.m_name)
, m_description(rhs.m_description)
{

}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

