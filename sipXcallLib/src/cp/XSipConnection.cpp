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
#include <os/OsReadLock.h>
#include <cp/XSipConnection.h>
#include <cp/XSipConnectionContext.h>

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

XSipConnection::XSipConnection(SipUserAgent& rSipUserAgent,
                               CpMediaInterfaceProvider* pMediaInterfaceProvider)
: m_instanceRWMutex(OsRWMutex::Q_FIFO)
, m_stateMachine(m_sipConnectionContext, rSipUserAgent, pMediaInterfaceProvider)
, m_rSipUserAgent(rSipUserAgent)
, m_pMediaInterfaceProvider(pMediaInterfaceProvider)
{
   m_stateMachine.setStateObserver(this); // register for state machine state change notifications
}

XSipConnection::~XSipConnection()
{
   m_stateMachine.setStateObserver(NULL);
}

/* ============================ MANIPULATORS ============================== */

OsStatus XSipConnection::acquire(const OsTime& rTimeout /*= OsTime::OS_INFINITY*/)
{
   return m_instanceRWMutex.acquireRead();
}

OsStatus XSipConnection::acquireExclusive()
{
   return m_instanceRWMutex.acquireWrite();
}

OsStatus XSipConnection::tryAcquire()
{
   return m_instanceRWMutex.tryAcquireRead();
}

OsStatus XSipConnection::release()
{
   return m_instanceRWMutex.releaseRead();
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
   OsReadLock lock(m_sipConnectionContext);
   sSipDialog = m_sipConnectionContext.m_sipDialog;
}

void XSipConnection::getSipCallId(UtlString& sSipCallId) const
{
   OsReadLock lock(m_sipConnectionContext);
   m_sipConnectionContext.m_sipDialog.getCallId(sSipCallId);
}

void XSipConnection::getRemoteUserAgent(UtlString& sRemoteUserAgent) const
{
   OsReadLock lock(m_sipConnectionContext);
   sRemoteUserAgent = m_sipConnectionContext.m_remoteUserAgent;
}

void XSipConnection::getMediaConnectionId(int& mediaConnID) const
{
   OsReadLock lock(m_sipConnectionContext);
   mediaConnID = m_sipConnectionContext.m_mediaConnectionId;
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
   OsReadLock lock(m_sipConnectionContext);
   return m_sipConnectionContext.m_sipDialog.compareDialogs(sSipDialog);
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void XSipConnection::handleStateEntry(ISipConnectionState::StateEnum state)
{

}

void XSipConnection::handleStateExit(ISipConnectionState::StateEnum state)
{

}

/* ============================ FUNCTIONS ================================= */
