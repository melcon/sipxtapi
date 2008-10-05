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
#include <cp/XCpAbstractConnection.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
const UtlContainableType XCpAbstractConnection::TYPE = "XCpAbstractConnection";

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XCpAbstractConnection::XCpAbstractConnection()
: m_memberMutex(OsMutex::Q_FIFO)
{

}

XCpAbstractConnection::~XCpAbstractConnection()
{

}

/* ============================ MANIPULATORS ============================== */

OsStatus XCpAbstractConnection::acquire(const OsTime& rTimeout /*= OsTime::OS_INFINITY*/)
{
   return m_memberMutex.acquire(rTimeout);
}

OsStatus XCpAbstractConnection::tryAcquire()
{
   return m_memberMutex.tryAcquire();
}

OsStatus XCpAbstractConnection::release()
{
   return m_memberMutex.release();
}

/* ============================ ACCESSORS ================================= */

unsigned XCpAbstractConnection::hash() const
{
   return (unsigned)this;
}

UtlContainableType XCpAbstractConnection::getContainableType() const
{
   return XCpAbstractConnection::TYPE;
}

/* ============================ INQUIRY =================================== */

int XCpAbstractConnection::compareTo(UtlContainable const* inVal) const
{
   int result;

   if (inVal->isInstanceOf(XCpAbstractConnection::TYPE))
   {
      result = hash() - inVal->hash();
   }
   else
   {
      result = -1; 
   }

   return result;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
