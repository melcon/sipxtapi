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
#include <cp/XCpCallIdUtil.h>
#include <cp/XCpCallStack.h>
#include <cp/XCpAbstractCall.h>
#include <cp/XCpCall.h>
#include <cp/XCpConference.h>
#include <cp/msg/AcGainFocusMsg.h>
#include <cp/msg/AcYieldFocusMsg.h>

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

XCpCallStack::XCpCallStack()
: m_sAbstractCallInFocus(NULL)
, m_focusMutex(OsMutex::Q_FIFO)
, m_memberMutex(OsMutex::Q_FIFO)
{

}

XCpCallStack::~XCpCallStack()
{

}

/* ============================ MANIPULATORS ============================== */

UtlBoolean XCpCallStack::findAbstractCall(const UtlString& sAbstractCallId,
                                                 OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   XCpCallIdUtil::ID_TYPE type = XCpCallIdUtil::getIdType(sAbstractCallId);
   UtlBoolean result = FALSE;
   switch(type)
   {
   case XCpCallIdUtil::ID_TYPE_CALL:
      {
         OsLock lock(m_memberMutex);
         // cannot reuse existing method, as OsPtrLock<XCpAbstractCall>& cannot be cast to OsPtrLock<XCpCall>&
         XCpCall* pCall = dynamic_cast<XCpCall*>(m_callMap.findValue(&sAbstractCallId));
         if (pCall)
         {
            ptrLock = pCall;
            return TRUE;
         }

         ptrLock = NULL;
         return FALSE;
      }
   case XCpCallIdUtil::ID_TYPE_CONFERENCE:
      {
         OsLock lock(m_memberMutex);
         // cannot reuse existing method, as OsPtrLock<XCpAbstractCall>& cannot be cast to OsPtrLock<XCpConference>&
         XCpConference* pConference = dynamic_cast<XCpConference*>(m_conferenceMap.findValue(&sAbstractCallId));
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

UtlBoolean XCpCallStack::findAbstractCall(const SipDialog& sSipDialog,
                                                 OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   OsPtrLock<XCpCall> ptrCallLock; // auto pointer lock
   OsPtrLock<XCpConference> ptrConferenceLock; // auto pointer lock
   UtlBoolean resFind = FALSE;

   // first try to find in calls
   resFind = findCall(sSipDialog, ptrCallLock);
   if (resFind)
   {
      ptrLock = ptrConferenceLock; // move lock
      return TRUE;
   }
   // not found, try conferences
   resFind = findConference(sSipDialog, ptrConferenceLock);
   if (resFind)
   {
      ptrLock = ptrConferenceLock; // move lock
      return TRUE;
   }

   return FALSE;
}

UtlBoolean XCpCallStack::findSomeAbstractCall(const UtlString& sAvoidAbstractCallId,
                                                     OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   OsLock lock(m_memberMutex);

   // try to get next call
   {
      UtlHashMapIterator callMapItor(m_callMap);
      XCpCall* pCall = NULL;

      while(callMapItor()) // go to next pair
      {
         pCall = dynamic_cast<XCpCall*>(callMapItor.value());
         if (pCall && sAvoidAbstractCallId.compareTo(pCall->getId()) != 0)
         {
            // we found some call and sAvoidAbstractCallId is different than its Id
            ptrLock = pCall; // lock call
            return TRUE;
         }
      }
   }

   // try to get next conference
   {
      UtlHashMapIterator conferenceMapItor(m_conferenceMap);
      XCpConference* pConference = NULL;
      while(conferenceMapItor()) // go to next pair
      {
         pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
         if (pConference && sAvoidAbstractCallId.compareTo(pConference->getId()) != 0)
         {
            // we found some conference and sAvoidAbstractCallId is different than its Id
            ptrLock = pConference; // lock conference
            return TRUE;
         }
      }
   }

   ptrLock = NULL;
   return FALSE;
}

UtlBoolean XCpCallStack::findCall(const UtlString& sId,
                                         OsPtrLock<XCpCall>& ptrLock) const
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

UtlBoolean XCpCallStack::findCall(const SipDialog& sSipDialog,
                                         OsPtrLock<XCpCall>& ptrLock) const
{
   OsLock lock(m_memberMutex);
   XCpCall* pNotEstablishedMatch = NULL;

   // iterate through hashmap and ask every call if it has given sip dialog
   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   // TODO: optimize speed by using list in hashmap indexed by call-id. Have to investigate if call-id switch in XCpCall is possible/desirable
   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         SipDialog::DialogMatchEnum matchResult = pCall->hasSipDialog(sSipDialog);
         if (matchResult == SipDialog::DIALOG_ESTABLISHED_MATCH ||
            matchResult == SipDialog::DIALOG_INITIAL_INITIAL_MATCH)
         {
            // perfect match, call-id and both tags match
            // initial match is also perfect if supplied dialog is initial
            ptrLock = pCall;
            return TRUE;
         }
         else if (matchResult != SipDialog::DIALOG_MISMATCH)
         {
            // partial match, call-id match but only 1 tag matches, 2nd tag is not present
            pNotEstablishedMatch = pCall; // lock it later
         }
      }
   }

   if (pNotEstablishedMatch)
   {
      ptrLock = pNotEstablishedMatch;
      return TRUE;
   }

   return FALSE;
}

UtlBoolean XCpCallStack::findConference(const UtlString& sId,
                                               OsPtrLock<XCpConference>& ptrLock) const
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

UtlBoolean XCpCallStack::findConference(const SipDialog& sSipDialog,
                                               OsPtrLock<XCpConference>& ptrLock) const
{
   OsLock lock(m_memberMutex);
   XCpConference* pNotEstablishedMatch = NULL;

   // iterate through hashmap and ask every conference if it has given sip dialog
   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         SipDialog::DialogMatchEnum matchResult = pConference->hasSipDialog(sSipDialog);
         if (matchResult == SipDialog::DIALOG_ESTABLISHED_MATCH ||
            matchResult == SipDialog::DIALOG_INITIAL_INITIAL_MATCH)
         {
            // perfect match, call-id and both tags match
            // initial match is also perfect if supplied dialog is initial
            ptrLock = pConference;
            return TRUE;
         }
         else if (matchResult != SipDialog::DIALOG_MISMATCH)
         {
            // partial match, call-id match but only 1 tag matches, 2nd tag is not present
            pNotEstablishedMatch = pConference; // lock it later
         }
      }
   }

   if (pNotEstablishedMatch)
   {
      ptrLock = pNotEstablishedMatch;
      return TRUE;
   }

   return FALSE;
}

UtlBoolean XCpCallStack::findHandlingAbstractCall(const SipMessage& rSipMessage,
                                                         OsPtrLock<XCpAbstractCall>& ptrLock) const
{
   SipDialog sipDialog(&rSipMessage);
   return findAbstractCall(sipDialog, ptrLock);
}

UtlBoolean XCpCallStack::push(XCpCall& call)
{
   OsLock lock(m_memberMutex);

   UtlCopyableContainable *pKey = call.getId().clone();
   UtlContainable *pResult = m_callMap.insertKeyAndValue(pKey, &call);
   if (pResult)
   {
      return TRUE;
   }
   else
   {
      delete pKey;
      pKey = NULL;
      return FALSE;
   }
}

UtlBoolean XCpCallStack::push(XCpConference& conference)
{
   OsLock lock(m_memberMutex);

   UtlCopyableContainable *pKey = conference.getId().clone();
   UtlContainable *pResult = m_conferenceMap.insertKeyAndValue(pKey, &conference);
   if (pResult)
   {
      return TRUE;
   }
   else
   {
      delete pKey;
      pKey = NULL;
      return FALSE;
   }
}

UtlBoolean XCpCallStack::deleteCall(const UtlString& sCallId)
{
   OsLock lock(m_memberMutex);
   // yield focus
   // nobody will be able to give us focus back after we yield it, because we hold m_memberMutex
   doYieldFocus(sCallId);

   // avoid findCall, as we don't need that much locking
   UtlContainable *pValue = NULL;
   UtlContainable *pKey = m_callMap.removeKeyAndValue(&sCallId, pValue);
   if(pKey)
   {
      XCpCall *pCall = dynamic_cast<XCpCall*>(pValue);
      if (pCall)
      {
         // call was found
         pCall->acquireExclusive(); // lock the call exclusively for delete
         delete pCall;
         pCall = NULL;
      }
      else
      {
         OsSysLog::add(FAC_CP, PRI_WARNING, "Unexpected state. Key was removed from call hashmap but value was NULL.");
      }
      delete pKey;
      pKey = NULL;
      return TRUE;
   }
   return FALSE;
}

UtlBoolean XCpCallStack::deleteConference(const UtlString& sConferenceId)
{
   OsLock lock(m_memberMutex);
   // yield focus
   // nobody will be able to give us focus back after we yield it, because we hold m_memberMutex
   doYieldFocus(sConferenceId);

   // avoid findConference, as we don't need that much locking
   UtlContainable *pValue = NULL;
   UtlContainable *pKey = m_conferenceMap.removeKeyAndValue(&sConferenceId, pValue);
   if(pKey)
   {
      XCpConference *pConference = dynamic_cast<XCpConference*>(pValue);
      if (pConference)
      {
         // conference was found
         pConference->acquireExclusive(); // lock the conference exclusively for delete
         delete pConference;
         pConference = NULL;
      }
      else
      {
         OsSysLog::add(FAC_CP, PRI_WARNING, "Unexpected state. Key was removed from conference hashmap but value was NULL.");
      }
      delete pKey;
      pKey = NULL;
      return TRUE;
   }
   return FALSE;
}

UtlBoolean XCpCallStack::deleteAbstractCall(const UtlString& sAbstractCallId)
{
   XCpCallIdUtil::ID_TYPE type = XCpCallIdUtil::getIdType(sAbstractCallId);
   UtlBoolean result = FALSE;

   switch(type)
   {
   case XCpCallIdUtil::ID_TYPE_CALL:
      {
         deleteCall(sAbstractCallId);
      }
   case XCpCallIdUtil::ID_TYPE_CONFERENCE:
      {
         deleteConference(sAbstractCallId);
      }
   default:
      break;
   }

   return result;
}

void XCpCallStack::deleteAllCalls()
{
   doYieldFocus(FALSE);
   OsLock lock(m_memberMutex);
   m_callMap.destroyAll();
}

void XCpCallStack::deleteAllConferences()
{
   doYieldFocus(FALSE);
   OsLock lock(m_memberMutex);
   m_conferenceMap.destroyAll();
}

OsStatus XCpCallStack::doGainFocus(const UtlString& sAbstractCallId,
                                          UtlBoolean bGainOnlyIfNoFocusedCall /*= FALSE*/)
{
#ifndef DISABLE_LOCAL_AUDIO
   OsStatus result = OS_FAILED;
   OsLock lock(m_focusMutex);

   if (!m_sAbstractCallInFocus.isNull())
   {
      // some call is focused
      if (bGainOnlyIfNoFocusedCall)
      {
         // some call is focused, then we don't want to focus
         return OS_SUCCESS;
      }
      result = doYieldFocus(sAbstractCallId, FALSE); // defocus focused call
   }

   {
      OsPtrLock<XCpAbstractCall> ptrLock; // scoped auto pointer lock
      UtlBoolean resFind = findAbstractCall(sAbstractCallId, ptrLock);
      if (resFind)
      {
         // send gain focus command to new call
         AcGainFocusMsg gainFocusCommand;
         // we found call and have a lock on it
         ptrLock->postMessage(gainFocusCommand);
         m_sAbstractCallInFocus = sAbstractCallId;
         result = OS_SUCCESS;
      }
   }

   return result;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpCallStack::doGainNextFocus(const UtlString& sAvoidAbstractCallId)
{
#ifndef DISABLE_LOCAL_AUDIO
   OsStatus result = OS_FAILED;
   OsLock lock(m_focusMutex); // need to hold lock of the whole time to ensure consistency

   if (m_sAbstractCallInFocus.isNull())
   {
      // no call has focus
      OsPtrLock<XCpAbstractCall> ptrLock; // scoped auto pointer lock
      UtlBoolean resFind = findSomeAbstractCall(sAvoidAbstractCallId, ptrLock);// avoids sAvoidAbstractCallId when looking for next call
      if (resFind)
      {
         // send gain focus command to new call
         AcGainFocusMsg gainFocusCommand;
         // we found call and have a lock on it
         ptrLock->postMessage(gainFocusCommand);
         m_sAbstractCallInFocus = ptrLock->getId();
         result = OS_SUCCESS;
      }
   }

   return result;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpCallStack::doYieldFocus(const UtlString& sAbstractCallId,
                                           UtlBoolean bShiftFocus /*= TRUE*/)
{
#ifndef DISABLE_LOCAL_AUDIO
   OsStatus result = OS_FAILED;
   OsLock lock(m_focusMutex); // need to hold lock of the whole time to ensure consistency

   if (m_sAbstractCallInFocus.compareTo(sAbstractCallId) == 0)
   {
      {
         OsPtrLock<XCpAbstractCall> ptrLock; // scoped auto pointer lock
         UtlBoolean resFind = findAbstractCall(m_sAbstractCallInFocus, ptrLock);
         if (resFind)
         {
            // send defocus command to old call
            AcYieldFocusMsg defocusCommand;
            // we found call and have a lock on it
            ptrLock->postMessage(defocusCommand);
            result = OS_SUCCESS;
         }
      }

      if (bShiftFocus)
      {
         UtlString sAvoidAbstractCallId(m_sAbstractCallInFocus);
         m_sAbstractCallInFocus.remove(0);
         result = doGainNextFocus(sAvoidAbstractCallId);
      }
      else
      {
         m_sAbstractCallInFocus.remove(0);
      }
   }

   return result;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpCallStack::doYieldFocus(UtlBoolean bShiftFocus /*= TRUE*/)
{
#ifndef DISABLE_LOCAL_AUDIO
   OsStatus result = OS_FAILED;
   OsLock lock(m_focusMutex); // need to hold lock of the whole time to ensure consistency

   if (!m_sAbstractCallInFocus.isNull())
   {
      {
         OsPtrLock<XCpAbstractCall> ptrLock; // scoped auto pointer lock
         UtlBoolean resFind = findAbstractCall(m_sAbstractCallInFocus, ptrLock);
         if (resFind)
         {
            // send defocus command to old call
            AcYieldFocusMsg defocusCommand;
            // we found call and have a lock on it
            ptrLock->postMessage(defocusCommand);
            result = OS_SUCCESS;
         }
      }

      if (bShiftFocus)
      {
         UtlString sAvoidAbstractCallId(m_sAbstractCallInFocus);
         m_sAbstractCallInFocus.remove(0);
         result = doGainNextFocus(sAvoidAbstractCallId);
      }
      else
      {
         m_sAbstractCallInFocus.remove(0);
      }
   }

   return result;
#else
   return OS_SUCCESS;
#endif
}

void XCpCallStack::shutdownAllCallThreads()
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         pCall->requestShutdown();
      }
   }
}

void XCpCallStack::shutdownAllConferenceThreads()
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         pConference->requestShutdown();
      }
   }
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

int XCpCallStack::getCallCount() const
{
   OsLock lock(m_memberMutex);
   int count = 0;

   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         count += pCall->getCallCount();
      }
   }

   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         count += pConference->getCallCount();
      }
   }

   return count;
}

OsStatus XCpCallStack::getCallIds(UtlSList& callIdList) const
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator callMapItor(m_callMap);
   XCpCall* pCall = NULL;

   while(callMapItor()) // go to next pair
   {
      pCall = dynamic_cast<XCpCall*>(callMapItor.value());
      if (pCall)
      {
         callIdList.insert(pCall->getId().clone());
      }
   }

   return OS_SUCCESS;
}

OsStatus XCpCallStack::getConferenceIds(UtlSList& conferenceIdList) const
{
   OsLock lock(m_memberMutex);

   UtlHashMapIterator conferenceMapItor(m_conferenceMap);
   XCpConference* pConference = NULL;
   while(conferenceMapItor()) // go to next pair
   {
      pConference = dynamic_cast<XCpConference*>(conferenceMapItor.value());
      if (pConference)
      {
         conferenceIdList.insert(pConference->getId().clone());
      }
   }

   return OS_SUCCESS;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

