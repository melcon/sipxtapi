//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <assert.h>
#include <stdlib.h>

// APPLICATION INCLUDES
#include <utl/UtlInit.h>
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <os/OsQueuedEvent.h>
#include <os/OsEventMsg.h>
#include "os/OsSysLog.h"
#include <os/OsLock.h>
#include <mi/CpMediaInterface.h>
#include <cp/CpCall.h>
#include <cp/CpMultiStringMessage.h>
#include <cp/CpIntMessage.h>
#include <ptapi/PtCall.h>
#include <ptapi/PtTerminalConnection.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define CALL_STACK_SIZE (24*1024)    // 24K stack for the call task

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CpCall::CpCall(CpCallManager* manager,
               CpMediaInterface* callMediaInterface,
               int callIndex,
               const char* callId)
               : OsServerTask("Call-%d", NULL, DEF_MAX_MSGS, DEF_PRIO, DEF_OPTIONS, CALL_STACK_SIZE)
               , m_memberMutex(OsMutex::Q_FIFO)
               , m_bindIPAddress("0.0.0.0")
{
   // add the call task name to a list so we can track leaked calls.
   UtlString strCallTaskName = getName();

   mCallInFocus = FALSE;
   mpManager = manager;

   mDropping = FALSE;
   mLocalHeld = FALSE;

   mCallIndex = callIndex;
   if(callId && callId[0])
   {
      setCallId(callId);
   }

   // Create the media processing channel
   mpMediaInterface = callMediaInterface;

   mCallState = PtCall::IDLE;
   mLocalConnectionState = PtEvent::CONNECTION_IDLE;
   mLocalTermConnectionState = PtTerminalConnection::IDLE;

   // Meta event intitialization
   mMetaEventId = 0;
   mMetaEventType = PtEvent::META_EVENT_NONE;
   mNumMetaEventCalls = 0;
   mpMetaEventCallIds = NULL;

   UtlString name = getName();
}

// Destructor
CpCall::~CpCall()
{
   waitUntilShutDown();

    // this is only failsafe, we release it when call is dropped from thread
   releaseMediaInterface();

   if(mpMetaEventCallIds)
   {
      delete[] mpMetaEventCallIds;
      mpMetaEventCallIds = NULL;
   }
}

/* ============================ MANIPULATORS ============================== */

void CpCall::setDropState(UtlBoolean state)
{
   mDropping = state;
}

void CpCall::setCallState(int responseCode, UtlString responseText, int state, int casue)
{
   mCallState = state;
}

UtlBoolean CpCall::handleMessage(OsMsg& eventMessage)
{
   int msgType = eventMessage.getMsgType();
   int msgSubType = eventMessage.getMsgSubType();
   //int key;
   //int hookState;
   CpMultiStringMessage* multiStringMessage = (CpMultiStringMessage*)&eventMessage;

   UtlBoolean processedMessage = TRUE;
   OsSysLog::add(FAC_CP, PRI_DEBUG, "CpCall::handleMessage message type: %d subtype %d\n", msgType, msgSubType);

   switch(msgType)
   {
   case OsMsg::PHONE_APP:

      switch(msgSubType)
      {
      case CallManager::CP_PLAY_AUDIO_TERM_CONNECTION:
         {
            int repeat = ((CpMultiStringMessage&)eventMessage).getInt1Data();
            UtlBoolean local = ((CpMultiStringMessage&)eventMessage).getInt2Data();
            UtlBoolean remote = ((CpMultiStringMessage&)eventMessage).getInt3Data();
            UtlBoolean mixWithMic = ((CpMultiStringMessage&)eventMessage).getInt4Data();
            int downScaling = ((CpMultiStringMessage&)eventMessage).getInt5Data();
            UtlString url;
            ((CpMultiStringMessage&)eventMessage).getString2Data(url);

            if(mpMediaInterface)
            {
               mpMediaInterface->playAudio(url.data(), repeat,
                  local, remote, mixWithMic, downScaling);
            }
         }
         break;

      case CallManager::CP_PLAY_BUFFER_TERM_CONNECTION:
         {
            int repeat = ((CpMultiStringMessage&)eventMessage).getInt2Data();
            UtlBoolean local = ((CpMultiStringMessage&)eventMessage).getInt3Data();
            UtlBoolean remote = ((CpMultiStringMessage&)eventMessage).getInt4Data();
            int buffer = ((CpMultiStringMessage&)eventMessage).getInt5Data();
            int bufSize = ((CpMultiStringMessage&)eventMessage).getInt6Data();
            int type = ((CpMultiStringMessage&)eventMessage).getInt7Data();
            void* pCookie = (void*)((CpMultiStringMessage&)eventMessage).getInt8Data();

            if(mpMediaInterface)
            {
               mpMediaInterface->playBuffer((char*)buffer,
                  bufSize, type, repeat, local, remote, false, 100, pCookie);
            }
         }
         break;
      case CallManager::CP_PAUSE_AUDIO_PLAYBACK_CONNECTION:
         {
            if(mpMediaInterface)
            {
               mpMediaInterface->pausePlayback();
            }
         }
         break;
      case CallManager::CP_RESUME_AUDIO_PLAYBACK_CONNECTION:
         {
            if(mpMediaInterface)
            {
               mpMediaInterface->resumePlayback();
            }
         }
         break;
      case CallManager::CP_STOP_AUDIO_TERM_CONNECTION:
         if(mpMediaInterface)
         {
            mpMediaInterface->stopAudio();
         }
         break;

      case CallManager::CP_DROP:
         {
            UtlString callId;
            int metaEventId = ((CpMultiStringMessage&)eventMessage).getInt1Data();
            ((CpMultiStringMessage&)eventMessage).getString1Data(callId);

            hangUp(callId, metaEventId);                                         
         }
         break;

      default:
         processedMessage = handleCallMessage(eventMessage);
         break;
      }

      break;
   case OsMsg::MP_CONNECTION_NOTF_MSG:
      handleConnectionNotfMessage(eventMessage);
      break;
   case OsMsg::MP_INTERFACE_NOTF_MSG:
      handleInterfaceNotfMessage(eventMessage);
      break;
   default:
      processedMessage = FALSE;
      osPrintf("Unknown TYPE %d of Call message subtype: %d\n", msgType, msgSubType);
      break;
   }

   //    osPrintf("exiting CpCall::handleMessage\n");
   return(processedMessage);
}

void CpCall::inFocus(int talking)
{
   mCallInFocus = TRUE;

   mLocalConnectionState = PtEvent::CONNECTION_ESTABLISHED;
   if (talking)
      mLocalTermConnectionState = PtTerminalConnection::TALKING;
   else
      mLocalTermConnectionState = PtTerminalConnection::IDLE;

   if(mpMediaInterface)
   {
      mpMediaInterface->giveFocus();
   }


}

void CpCall::outOfFocus()
{
   mCallInFocus = FALSE;

   if(mpMediaInterface)
   {
      mpMediaInterface->defocus();
   }
}

void CpCall::localHold()
{
   if(!mLocalHeld)
   {
      mLocalHeld = TRUE;
      mpManager->yieldFocus(this);
      /*
      // Post a message to the callManager to change focus
      CpIntMessage localHoldMessage(CallManager::CP_YIELD_FOCUS,
      (int)this);
      mLocalTermConnectionState = PtTerminalConnection::HELD;
      mpManager->postMessage(localHoldMessage);
      */
   }
}

void CpCall::hangUp(UtlString callId, int metaEventId)
{
   mDropping = TRUE;
   mLocalConnectionState = PtEvent::CONNECTION_DISCONNECTED;
   mLocalTermConnectionState = PtTerminalConnection::DROPPED;

   if (metaEventId > 0)
      setMetaEvent(metaEventId, PtEvent::META_CALL_ENDING, 0, 0);
   else
      startMetaEvent(mpManager->getNewMetaEventId(), PtEvent::META_CALL_ENDING, 0, 0);

   onHook();
}

void CpCall::setLocalConnectionState(int newState)
{
   mLocalConnectionState = newState;
}

/* ============================ ACCESSORS ================================= */

int CpCall::getCallIndex()
{
   return(mCallIndex);
}

int CpCall::getCallState()
{
   return(mCallState);
}

void CpCall::getCallId(UtlString& callId)
{
   OsLock lock(m_memberMutex);
   callId = mCallId;
}

void CpCall::setCallId(const char* callId)
{
   OsLock lock(m_memberMutex);
   mCallId.remove(0);
   if(callId) mCallId.append(callId);
}

int CpCall::getLocalConnectionStateFromPt(int state)
{
   int newState;

   switch(state)
   {
   case PtEvent::CONNECTION_CREATED:
   case PtEvent::CONNECTION_INITIATED:
      newState = Connection::CONNECTION_INITIATED;
      break;

   case PtEvent::CONNECTION_ALERTING:
      newState = Connection::CONNECTION_ALERTING;
      break;

   case PtEvent::CONNECTION_DISCONNECTED:
      newState = Connection::CONNECTION_DISCONNECTED;
      break;

   case PtEvent::CONNECTION_FAILED:
      newState = Connection::CONNECTION_FAILED;
      break;

   case PtEvent::CONNECTION_DIALING:
      newState = Connection::CONNECTION_DIALING;
      break;

   case PtEvent::CONNECTION_ESTABLISHED:
      newState = Connection::CONNECTION_ESTABLISHED;
      break;

   case PtEvent::CONNECTION_NETWORK_ALERTING:
      newState = Connection::CONNECTION_NETWORK_ALERTING;
      break;

   case PtEvent::CONNECTION_NETWORK_REACHED:
      newState = Connection::CONNECTION_NETWORK_REACHED;
      break;

   case PtEvent::CONNECTION_OFFERED:
      newState = Connection::CONNECTION_OFFERING;
      break;

   case PtEvent::CONNECTION_QUEUED:
      newState = Connection::CONNECTION_QUEUED;
      break;

   default:
      newState = Connection::CONNECTION_UNKNOWN;
      break;

   }

   return newState;
}

void CpCall::setMetaEvent(int metaEventId, int metaEventType,
                          int numCalls, const char* metaEventCallIds[])
{
   if (mMetaEventId != 0 || mMetaEventType != PtEvent::META_EVENT_NONE)
      stopMetaEvent();

   mMetaEventId = metaEventId;
   mMetaEventType = metaEventType;

   if(mpMetaEventCallIds)
   {
      delete[] mpMetaEventCallIds;
      mpMetaEventCallIds = NULL;
   }

   if (numCalls > 0)
   {
      mNumMetaEventCalls = numCalls;
      mpMetaEventCallIds = new UtlString[numCalls];
      for(int i = 0; i < numCalls; i++)
      {
         if (metaEventCallIds)
            mpMetaEventCallIds[i] = metaEventCallIds[i];
         else
            mpMetaEventCallIds[i] = mCallId.data();
      }
   }
}

void CpCall::startMetaEvent(int metaEventId,
                            int metaEventType,
                            int numCalls,
                            const char* metaEventCallIds[],
                            int remoteIsCallee)
{
   setMetaEvent(metaEventId, metaEventType, numCalls, metaEventCallIds);
}

void CpCall::getMetaEvent(int& metaEventId, int& metaEventType,
                          int& numCalls, const UtlString* metaEventCallIds[]) const
{
   metaEventId = mMetaEventId;
   metaEventType = mMetaEventType;
   numCalls = mNumMetaEventCalls;
   *metaEventCallIds = mpMetaEventCallIds;
}

void CpCall::stopMetaEvent(int remoteIsCallee)
{
   // Clear the event info
   mMetaEventId = 0;
   mMetaEventType = PtEvent::META_EVENT_NONE;

   if(mpMetaEventCallIds)
   {
      delete[] mpMetaEventCallIds;
      mpMetaEventCallIds = NULL;
   }
}

void CpCall::setCallType(int callType)
{
   mCallType = callType;
}

int CpCall::getCallType() const
{
   return(mCallType);
}

void CpCall::setTargetCallId(const char* targetCallId)
{
   if(targetCallId && * targetCallId) mTargetCallId = targetCallId;
}

void CpCall::getTargetCallId(UtlString& targetCallId) const
{
   targetCallId = mTargetCallId;
}

void CpCall::setOriginalCallId(const char* originalCallId)
{
   if(originalCallId && * originalCallId) mOriginalCallId = originalCallId;
}

void CpCall::getOriginalCallId(UtlString& originalCallId) const
{
   originalCallId = mOriginalCallId;
}
/* ============================ INQUIRY =================================== */

UtlBoolean CpCall::isCallIdSet()
{
   OsLock lock(m_memberMutex);
   return(!mCallId.isNull());
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

void CpCall::postTaoListenerMessage(int responseCode,
                                    UtlString responseText,
                                    int eventId,
                                    int type,
                                    int cause,
                                    int remoteIsCallee,
                                    UtlString remoteAddress,
                                    int isRemote,
                                    UtlString targetCallId)
{
   if (type == TERMINAL_CONNECTION_STATE)
      mLocalTermConnectionState = tcStateFromEventId(eventId);
}

int CpCall::tcStateFromEventId(int eventId)
{
   int state;

   switch(eventId)
   {
   case PtEvent::TERMINAL_CONNECTION_CREATED:
   case PtEvent::TERMINAL_CONNECTION_IDLE:
      state = PtTerminalConnection::IDLE;
      break;

   case PtEvent::TERMINAL_CONNECTION_HELD:
      state = PtTerminalConnection::HELD;
      break;

   case PtEvent::TERMINAL_CONNECTION_RINGING:
      state = PtTerminalConnection::RINGING;
      break;

   case PtEvent::TERMINAL_CONNECTION_TALKING:
      state = PtTerminalConnection::TALKING;
      break;

   case PtEvent::TERMINAL_CONNECTION_IN_USE:
      state = PtTerminalConnection::IN_USE;
      break;

   case PtEvent::TERMINAL_CONNECTION_DROPPED:
      state = PtTerminalConnection::DROPPED;
      break;

   default:
      state = PtTerminalConnection::UNKNOWN;
      break;
   }

   return state;
}

UtlString CpCall::getBindIPAddress() const
{
   OsLock lock(m_memberMutex);
   return m_bindIPAddress;
}

void CpCall::setBindIPAddress(const UtlString& val)
{
   OsLock lock(m_memberMutex);
   m_bindIPAddress = val;
}

void CpCall::releaseMediaInterface()
{
   if(mpMediaInterface)
   {
      mpMediaInterface->release();
      mpMediaInterface = NULL;
   }
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

