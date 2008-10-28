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
#include <cp/XCpAbstractCall.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
const int XCpAbstractCall::CALL_MAX_REQUEST_MSGS = 200;
const UtlContainableType XCpAbstractCall::TYPE = "XCpAbstractCall";

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XCpAbstractCall::XCpAbstractCall(const UtlString& sId,
                                 SipUserAgent& rSipUserAgent,
                                 CpMediaInterfaceFactory& rMediaInterfaceFactory)
: OsServerTask("XCpAbstractCall-%d", NULL, CALL_MAX_REQUEST_MSGS)
, m_memberMutex(OsMutex::Q_FIFO)
, m_sId(sId)
, m_rSipUserAgent(rSipUserAgent)
, m_rMediaInterfaceFactory(rMediaInterfaceFactory)
{

}

XCpAbstractCall::~XCpAbstractCall()
{
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean XCpAbstractCall::handleMessage(OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;

   switch (rRawMsg.getMsgType())
   {
   case 1:
   default:
      break;
   }

   return bResult;
}

OsStatus XCpAbstractCall::audioToneStart(int iToneId,
                                         UtlBoolean bLocal,
                                         UtlBoolean bRemote)
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioToneStop()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioFilePlay(const UtlString& audioFile,
                                        UtlBoolean bRepeat,
                                        UtlBoolean bLocal,
                                        UtlBoolean bRemote,
                                        UtlBoolean bMixWithMic /*= FALSE*/,
                                        int iDownScaling /*= 100*/,
                                        void* pCookie /*= NULL*/)
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioBufferPlay(const void* pAudiobuf,
                                          size_t iBufSize,
                                          int iType,
                                          UtlBoolean bRepeat,
                                          UtlBoolean bLocal,
                                          UtlBoolean bRemote,
                                          void* pCookie /*= NULL*/)
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioStop()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::pauseAudioPlayback()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::resumeAudioPlayback()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioRecordStart(const UtlString& sFile)
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioRecordStop()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::holdLocalConnection()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::unholdLocalConnection()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::acquire(const OsTime& rTimeout /*= OsTime::OS_INFINITY*/)
{
   return m_memberMutex.acquire(rTimeout);
}

OsStatus XCpAbstractCall::tryAcquire()
{
   return m_memberMutex.tryAcquire();
}

OsStatus XCpAbstractCall::release()
{
   return m_memberMutex.release();
}

/* ============================ ACCESSORS ================================= */

unsigned XCpAbstractCall::hash() const
{
   return (unsigned)this;
}

UtlContainableType XCpAbstractCall::getContainableType() const
{
   return XCpAbstractCall::TYPE;
}

UtlString XCpAbstractCall::getId() const
{
   return m_sId;
}

/* ============================ INQUIRY =================================== */

int XCpAbstractCall::compareTo(UtlContainable const* inVal) const
{
   int result;

   if (inVal->isInstanceOf(XCpAbstractCall::TYPE))
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

