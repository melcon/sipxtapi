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
#include <os/OsLock.h>
#include <os/OsPtrLock.h>
#include <utl/UtlHashMapIterator.h>
#include <utl/UtlInt.h>
#include <cp/XCpCallManager.h>
#include <cp/XCpAbstractCall.h>
#include <cp/XCpCall.h>
#include <cp/XCpConference.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
const char* callIdPrefix = "call_";
const char* conferenceIdPrefix = "conf_";
const char* sipCallIdPrefix = "s";

// STATIC VARIABLE INITIALIZATIONS
const int XCpCallManager::CALLMANAGER_MAX_REQUEST_MSGS = 2000;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XCpCallManager::XCpCallManager(UtlBoolean bDoNotDisturb,
                               UtlBoolean bEnableICE,
                               int rtpPortStart,
                               int rtpPortEnd)
: OsServerTask("XCallManager-%d", NULL, CALLMANAGER_MAX_REQUEST_MSGS)
, m_callIdGenerator(callIdPrefix)
, m_conferenceIdGenerator(conferenceIdPrefix)
, m_sipCallIdGenerator(sipCallIdPrefix)
, m_bDoNotDisturb(bDoNotDisturb)
, m_bEnableICE(bEnableICE)
, m_rtpPortStart(rtpPortStart)
, m_rtpPortEnd(rtpPortEnd)
, m_memberMutex(OsMutex::Q_FIFO)
{

}

XCpCallManager::~XCpCallManager()
{
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean XCpCallManager::handleMessage(OsMsg& rRawMsg)
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

UtlBoolean XCpCallManager::findAbstractCallById(const UtlString& sId, OsPtrLock<XCpAbstractCall>& ptrLock)
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   UtlBoolean result = FALSE;
   switch(type)
   {
   case XCpCallManager::ID_TYPE_CALL:
      {
         // cannot reuse existing method, as OsPtrLock<XCpAbstractCall>& cannot be cast to OsPtrLock<XCpCall>&
         OsLock lock(m_memberMutex);
         XCpCall* pCall = dynamic_cast<XCpCall*>(m_callMap.findValue(&sId));
         if (pCall)
         {
            ptrLock = pCall;
            return TRUE;
         }

         ptrLock = NULL;
         return FALSE;
      }
   case XCpCallManager::ID_TYPE_CONFERENCE:
      {
         // cannot reuse existing method, as OsPtrLock<XCpAbstractCall>& cannot be cast to OsPtrLock<XCpConference>&
         OsLock lock(m_memberMutex);
         XCpConference* pConference = dynamic_cast<XCpConference*>(m_conferenceMap.findValue(&sId));
         if (pConference)
         {
            ptrLock = pConference;
            return TRUE;
         }

         ptrLock = NULL;
         return FALSE;
      }
   default:
      break;
   }

   ptrLock = NULL;
   return result;
}

UtlBoolean XCpCallManager::findAbstractCallBySipCallId(const UtlString& sSipCallId,
                                                       const UtlString& sFromTag,
                                                       const UtlString& sToTag,
                                                       OsPtrLock<XCpAbstractCall>& ptrLock)
{
   return FALSE;
}

UtlBoolean XCpCallManager::findCall(const UtlString& sId, OsPtrLock<XCpCall>& ptrLock)
{
   OsLock lock(m_memberMutex);
   XCpCall* pCall = dynamic_cast<XCpCall*>(m_callMap.findValue(&sId));
   if (pCall)
   {
      ptrLock = pCall;
      return TRUE;
   }
   
   ptrLock = NULL;
   return FALSE;
}

UtlBoolean XCpCallManager::findConference(const UtlString& sId, OsPtrLock<XCpConference>& ptrLock)
{
   OsLock lock(m_memberMutex);
   XCpConference* pConference = dynamic_cast<XCpConference*>(m_conferenceMap.findValue(&sId));
   if (pConference)
   {
      ptrLock = pConference;
      return TRUE;
   }

   ptrLock = NULL;
   return FALSE;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

UtlString XCpCallManager::getNewCallId()
{
   return m_callIdGenerator.getNewCallId();
}

UtlString XCpCallManager::getNewConferenceId()
{
   return m_conferenceIdGenerator.getNewCallId();
}

UtlString XCpCallManager::getNewSipCallId()
{
   return m_sipCallIdGenerator.getNewCallId();
}

UtlBoolean XCpCallManager::isCallId(const UtlString& sId)
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   return type == XCpCallManager::ID_TYPE_CALL;
}

UtlBoolean XCpCallManager::isConferenceId(const UtlString& sId)
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   return type == XCpCallManager::ID_TYPE_CONFERENCE;
}

UtlBoolean XCpCallManager::isSipCallId(const UtlString& sId)
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   return type == XCpCallManager::ID_TYPE_SIP;
}

XCpCallManager::ID_TYPE XCpCallManager::getIdType(const UtlString& sId)
{
   if (sId.first(callIdPrefix) >= 0)
   {
      return XCpCallManager::ID_TYPE_CALL;
   }
   else if (sId.first(conferenceIdPrefix) >= 0)
   {
      return XCpCallManager::ID_TYPE_CONFERENCE;
   }
   else return XCpCallManager::ID_TYPE_SIP;
}

/* ============================ FUNCTIONS ================================= */
