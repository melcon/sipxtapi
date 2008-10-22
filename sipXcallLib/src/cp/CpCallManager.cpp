//
// Copyright (C) 2005-2006 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Daniel Petrie dpetrie AT SIPez DOT com

// SYSTEM INCLUDES
#include "os/OsDefs.h"
#include <assert.h>

// APPLICATION INCLUDES
#include <os/OsLock.h>
#include <os/OsDateTime.h>
#include <os/OsSocket.h>
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <os/OsProcess.h>
#include <cp/CpCallManager.h>
#include <cp/CpCall.h>
#include <net/NetMd5Codec.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
#ifdef __pingtel_on_posix__ /* [ */
const int    CpCallManager::CALLMANAGER_MAX_REQUEST_MSGS = 6000;
#else
const int    CpCallManager::CALLMANAGER_MAX_REQUEST_MSGS = 1000;
#endif  

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CpCallManager::CpCallManager(const char* taskName,
                             const char* callIdPrefix,
                             int rtpPortStart,
                             int rtpPortEnd) :
OsServerTask(taskName, NULL, CALLMANAGER_MAX_REQUEST_MSGS),
mCallListMutex(OsMutex::Q_FIFO),
m_callIdGenerator(callIdPrefix != NULL ? callIdPrefix : "c"),
m_sipCallIdGenerator("s"),
mCallIndices()
{
   mDoNotDisturbFlag = FALSE;
   mMsgWaitingFlag = FALSE;
   mOfferedTimeOut = 0;

   mRtpPortStart = rtpPortStart;
   mRtpPortEnd = rtpPortEnd;

   mLastMetaEventId = 0;
   mbEnableICE = false;
}

// Destructor
CpCallManager::~CpCallManager()
{
}

/* ============================ MANIPULATORS ============================== */

void CpCallManager::getNewCallId(UtlString* callId)
{
   *callId = m_callIdGenerator.getNewCallId();
}

void CpCallManager::getNewSessionId(UtlString* callId)
{
   *callId = m_sipCallIdGenerator.getNewCallId();
}

void CpCallManager::appendCall(CpCall* call)
{
   OsWriteLock lock(mCallListMutex);
   UtlInt* callCollectable = new UtlInt((int)call);
   mCallList.append(callCollectable);
}

void CpCallManager::pushCall(CpCall* call)
{
   OsWriteLock lock(mCallListMutex);
   UtlInt* callCollectable = new UtlInt((int)call);
   mCallList.insertAt(0, callCollectable);
}


void CpCallManager::setDoNotDisturb(int flag)
{
   mDoNotDisturbFlag = flag;
}

void CpCallManager::setMessageWaiting(int flag)
{
   mMsgWaitingFlag = flag;
}

void CpCallManager::setOfferedTimeout(int milisec)
{
   mOfferedTimeOut = milisec;
}

void CpCallManager::enableIce(UtlBoolean bEnable) 
{
   mbEnableICE = bEnable;
}


void CpCallManager::setVoiceQualityReportTarget(const char* szTargetSipUrl) 
{
   mVoiceQualityReportTarget = szTargetSipUrl;
}


/* ============================ ACCESSORS ================================= */

int CpCallManager::getNewMetaEventId()
{
   mLastMetaEventId++;
   return(mLastMetaEventId);
}

UtlBoolean CpCallManager::isIceEnabled() const
{
   return mbEnableICE;
}


UtlBoolean CpCallManager::getVoiceQualityReportTarget(UtlString& reportSipUrl) 
{
   UtlBoolean bRC = false;

   if (!mVoiceQualityReportTarget.isNull())
   {
      reportSipUrl = mVoiceQualityReportTarget;
      bRC = true;
   }

   return bRC;
}

/* ============================ INQUIRY =================================== */
UtlBoolean CpCallManager::isCallStateLoggingEnabled()
{
   return(mCallStateLogEnabled);
}


/* //////////////////////////// PROTECTED ///////////////////////////////// */

int CpCallManager::aquireCallIndex()
{
   int index = 0;
   UtlInt matchCallIndexColl;

   // Find the first unused slot
   UtlInt* existingCallIndex = NULL;
   do
   {
      index++;
      matchCallIndexColl.setValue(index);
      existingCallIndex = (UtlInt*) mCallIndices.find(&matchCallIndexColl);

   }
   while(existingCallIndex);

   // Insert the new one
   mCallIndices.insert(new UtlInt(matchCallIndexColl));
   return(index);
}

void CpCallManager::releaseCallIndex(int callIndex)
{
   if(callIndex > 0)
   {
      UtlInt matchCallIndexColl(callIndex);
      UtlInt* callIndexColl = NULL;
      callIndexColl = (UtlInt*) mCallIndices.remove(&matchCallIndexColl);

      if(callIndexColl) delete callIndexColl;
      callIndexColl = NULL;
   }
}


/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

