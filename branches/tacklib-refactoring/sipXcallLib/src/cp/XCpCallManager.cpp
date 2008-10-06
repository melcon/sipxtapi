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
const char* callIdPrefix = "call_"; // id prefix for all calls. Only used internally.
const char* conferenceIdPrefix = "conf_"; // id prefix for all conferences. Only used internally.
const char* sipCallIdPrefix = "s"; // prefix of sip call-id sent in sip messages.

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

// m_memberMutex lock is assumed
UtlBoolean XCpCallManager::findAbstractCallById(const UtlString& sId, OsPtrLock<XCpAbstractCall>& ptrLock)
{
   XCpCallManager::ID_TYPE type = getIdType(sId);
   UtlBoolean result = FALSE;
   switch(type)
   {
   case XCpCallManager::ID_TYPE_CALL:
      {
         // cannot reuse existing method, as OsPtrLock<XCpAbstractCall>& cannot be cast to OsPtrLock<XCpCall>&
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

// m_memberMutex lock is assumed
UtlBoolean XCpCallManager::findAbstractCallBySipDialog(const UtlString& sSipCallId,
                                                       const UtlString& sLocalTag,
                                                       const UtlString& sRemoteTag,
                                                       OsPtrLock<XCpAbstractCall>& ptrLock)
{
   // iterate through hashmap and ask every call if it has given sip dialog
   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   // TODO: optimize speed by using list in hashmap indexed by call-id. Have to investigate if call-id switch in XCpCall is possible/desirable
   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall && pCall->hasSipDialog(sSipCallId, sLocalTag, sRemoteTag))
      {
         ptrLock = pCall;
         return TRUE;
      }
   }

   // iterate through hashmap and ask every conference if it has given sip dialog
   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference && pConference->hasSipDialog(sSipCallId, sLocalTag, sRemoteTag))
      {
         ptrLock = pConference;
         return TRUE;
      }
   }

   return FALSE;
}

// m_memberMutex lock is assumed
UtlBoolean XCpCallManager::findCall(const UtlString& sId, OsPtrLock<XCpCall>& ptrLock)
{
   XCpCall* pCall = dynamic_cast<XCpCall*>(m_callMap.findValue(&sId));
   if (pCall)
   {
      ptrLock = pCall;
      return TRUE;
   }

   ptrLock = NULL;
   return FALSE;
}

// m_memberMutex lock is assumed
UtlBoolean XCpCallManager::findConference(const UtlString& sId, OsPtrLock<XCpConference>& ptrLock)
{
   XCpConference* pConference = dynamic_cast<XCpConference*>(m_conferenceMap.findValue(&sId));
   if (pConference)
   {
      ptrLock = pConference;
      return TRUE;
   }

   ptrLock = NULL;
   return FALSE;
}

/* ============================ FUNCTIONS ================================= */
