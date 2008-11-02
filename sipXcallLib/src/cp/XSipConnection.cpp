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
#include <cp/XSipConnection.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
const UtlContainableType XSipConnection::TYPE = "XSipConnection";

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XSipConnection::XSipConnection()
: m_memberMutex(OsMutex::Q_FIFO)
{

}

XSipConnection::~XSipConnection()
{

}

/* ============================ MANIPULATORS ============================== */

OsStatus XSipConnection::acquire(const OsTime& rTimeout /*= OsTime::OS_INFINITY*/)
{
   return m_memberMutex.acquire(rTimeout);
}

OsStatus XSipConnection::tryAcquire()
{
   return m_memberMutex.tryAcquire();
}

OsStatus XSipConnection::release()
{
   return m_memberMutex.release();
}

/* ============================ ACCESSORS ================================= */

unsigned XSipConnection::hash() const
{
   return (unsigned)this;
}

UtlContainableType XSipConnection::getContainableType() const
{
   return XSipConnection::TYPE;
}

void XSipConnection::getSipDialog(SipDialog& sSipDialog) const
{
   // TODO: implement
   sSipDialog = SipDialog();
}

void XSipConnection::getSipCallId(UtlString& sSipCallId) const
{
   // TODO: implement
   sSipCallId.remove(0);
}

void XSipConnection::getRemoteUserAgent(UtlString& sRemoteUserAgent) const
{
   // TODO: implement
   sRemoteUserAgent.remove(0);
}

void XSipConnection::getMediaConnectionId(int& mediaConnID) const
{
   // TODO: implement
   mediaConnID = -1;
}

/* ============================ INQUIRY =================================== */

int XSipConnection::compareTo(UtlContainable const* inVal) const
{
   int result;

   if (inVal->isInstanceOf(XSipConnection::TYPE))
   {
      result = hash() - inVal->hash();
   }
   else
   {
      result = -1; 
   }

   return result;
}

SipDialog::DialogMatchEnum XSipConnection::compareSipDialog(const SipDialog& sSipDialog) const
{
   return SipDialog::DIALOG_MISMATCH;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
