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
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <os/OsMsgQ.h>
#include <os/OsPtrLock.h>
#include <os/OsIntPtrMsg.h>
#include <net/SipMessage.h>
#include <mi/CpMediaInterfaceFactory.h>
#include <mi/CpMediaInterface.h>
#include <cp/XCpAbstractCall.h>
#include <cp/XSipConnection.h>
#include <cp/CpNotificationMsgDef.h>
#include <cp/CpMessageTypes.h>
#include <cp/msg/AcCommandMsg.h>
#include <cp/msg/AcNotificationMsg.h>
#include <cp/msg/AcAudioBufferPlayMsg.h>
#include <cp/msg/AcAudioFilePlayMsg.h>
#include <cp/msg/AcAudioPausePlaybackMsg.h>
#include <cp/msg/AcAudioResumePlaybackMsg.h>
#include <cp/msg/AcAudioStopPlaybackMsg.h>
#include <cp/msg/AcAudioRecordStartMsg.h>
#include <cp/msg/AcAudioRecordStopMsg.h>
#include <cp/msg/AcAudioToneStartMsg.h>
#include <cp/msg/AcAudioToneStopMsg.h>
#include <cp/msg/CmGainFocusMsg.h>
#include <cp/msg/CmYieldFocusMsg.h>
#include <cp/msg/CpTimerMsg.h>


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
                                 CpMediaInterfaceFactory& rMediaInterfaceFactory,
                                 OsMsgQ& rCallManagerQueue,
                                 CpCallStateEventListener* pCallEventListener,
                                 SipInfoStatusEventListener* pInfoStatusEventListener,
                                 SipSecurityEventListener* pSecurityEventListener,
                                 CpMediaEventListener* pMediaEventListener)
: OsServerTask("XCpAbstractCall-%d", NULL, CALL_MAX_REQUEST_MSGS)
, m_memberMutex(OsMutex::Q_FIFO)
, m_sId(sId)
, m_rSipUserAgent(rSipUserAgent)
, m_rMediaInterfaceFactory(rMediaInterfaceFactory)
, m_rCallManagerQueue(rCallManagerQueue)
, m_pMediaInterface(NULL)
, m_bIsFocused(FALSE)
, m_instanceRWMutex(OsRWMutex::Q_FIFO)
, m_sipTagGenerator()
, m_pCallEventListener(pCallEventListener)
, m_pInfoStatusEventListener(pInfoStatusEventListener)
, m_pSecurityEventListener(pSecurityEventListener)
, m_pMediaEventListener(pMediaEventListener)
{

}

XCpAbstractCall::~XCpAbstractCall()
{
   waitUntilShutDown();
   // release media interface if its still present. This should never happen. Only here as the last resort.
   releaseMediaInterface();
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean XCpAbstractCall::handleMessage(OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;

   switch (rRawMsg.getMsgType())
   {
   case CpMessageTypes::AC_COMMAND:
      return handleCommandMessage((const AcCommandMsg&)rRawMsg);
   case CpMessageTypes::AC_NOTIFICATION:
      return handleNotificationMessage((const AcNotificationMsg&)rRawMsg);
   case OsMsg::PHONE_APP:
      return handlePhoneAppMessage(rRawMsg);
   case CpTimerMsg::OS_TIMER_MSG:
      return handleTimerMessage((const CpTimerMsg&)rRawMsg);
   case OsMsg::MP_CONNECTION_NOTF_MSG:
      return handleConnectionNotfMessage((const OsIntPtrMsg&)rRawMsg);
   case OsMsg::MP_INTERFACE_NOTF_MSG:
      return handleInterfaceNotfMessage((const OsIntPtrMsg&)rRawMsg);
   case OsMsg::OS_EVENT: // timer event
   default:
      break;
   }

   return bResult;
}

OsStatus XCpAbstractCall::audioToneStart(int iToneId,
                                         UtlBoolean bLocal,
                                         UtlBoolean bRemote)
{
   AcAudioToneStartMsg audioToneStartMsg(iToneId, bLocal, bRemote);
   return postMessage(audioToneStartMsg);
}

OsStatus XCpAbstractCall::audioToneStop()
{
   AcAudioToneStopMsg audioToneStopMsg;
   return postMessage(audioToneStopMsg);
}

OsStatus XCpAbstractCall::audioFilePlay(const UtlString& audioFile,
                                        UtlBoolean bRepeat,
                                        UtlBoolean bLocal,
                                        UtlBoolean bRemote,
                                        UtlBoolean bMixWithMic /*= FALSE*/,
                                        int iDownScaling /*= 100*/,
                                        void* pCookie /*= NULL*/)
{
   AcAudioFilePlayMsg audioFilePlayMsg(audioFile, bRepeat, bLocal, bRemote, bMixWithMic, iDownScaling, pCookie);
   return postMessage(audioFilePlayMsg);
}

OsStatus XCpAbstractCall::audioBufferPlay(const void* pAudiobuf,
                                          size_t iBufSize,
                                          int iType,
                                          UtlBoolean bRepeat,
                                          UtlBoolean bLocal,
                                          UtlBoolean bRemote,
                                          UtlBoolean bMixWithMic /*= FALSE*/,
                                          int iDownScaling /*= 100*/,
                                          void* pCookie /*= NULL*/)
{
   AcAudioBufferPlayMsg audioBufferPlayMsg(pAudiobuf, iBufSize, iType, bRepeat, bLocal,
      bRemote, bMixWithMic, iDownScaling, pCookie);
   return postMessage(audioBufferPlayMsg);
}

OsStatus XCpAbstractCall::audioStopPlayback()
{
   AcAudioStopPlaybackMsg audioStopPlaybackMsg;
   return postMessage(audioStopPlaybackMsg);
}

OsStatus XCpAbstractCall::pauseAudioPlayback()
{
   AcAudioPausePlaybackMsg audioPausePlaybackMsg;
   return postMessage(audioPausePlaybackMsg);
}

OsStatus XCpAbstractCall::resumeAudioPlayback()
{
   AcAudioResumePlaybackMsg audioResumePlaybackMsg;
   return postMessage(audioResumePlaybackMsg);
}

OsStatus XCpAbstractCall::audioRecordStart(const UtlString& sFile)
{
   AcAudioRecordStartMsg audioRecordStartMsg(sFile);
   return postMessage(audioRecordStartMsg);
}

OsStatus XCpAbstractCall::audioRecordStop()
{
   AcAudioRecordStopMsg audioRecordStopMsg;
   return postMessage(audioRecordStopMsg);
}

OsStatus XCpAbstractCall::acquireExclusive()
{
   return m_instanceRWMutex.acquireWrite();
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

OsStatus XCpAbstractCall::getRemoteUserAgent(const SipDialog& sipDialog, UtlString& userAgent) const
{
   OsStatus result = OS_INVALID;

   OsPtrLock<XSipConnection> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConnection(sipDialog, ptrLock);
   if (resFind)
   {
      ptrLock->getRemoteUserAgent(userAgent);
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpAbstractCall::getMediaConnectionId(const SipDialog& sipDialog, int& mediaConnID) const
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XSipConnection> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConnection(sipDialog, ptrLock);
   if (resFind)
   {
      mediaConnID = ptrLock->getMediaConnectionId();
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpAbstractCall::getSipDialog(const SipDialog& sipDialog,
                                       SipDialog& sOutputSipDialog) const
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XSipConnection> ptrLock; // auto pointer lock
   UtlBoolean resFind = findConnection(sipDialog, ptrLock);
   if (resFind)
   {
      ptrLock->getSipDialog(sOutputSipDialog);
      result = OS_SUCCESS;
   }

   return result;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

UtlBoolean XCpAbstractCall::handleCommandMessage(const AcCommandMsg& rRawMsg)
{
   switch ((AcCommandMsg::SubTypeEnum)rRawMsg.getMsgSubType())
   {
   case AcCommandMsg::AC_GAIN_FOCUS:
      handleGainFocus((const AcGainFocusMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_YIELD_FOCUS:
      handleDefocus((const AcYieldFocusMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_BUFFER_PLAY:
      handleAudioBufferPlay((const AcAudioBufferPlayMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_FILE_PLAY:
      handleAudioFilePlay((const AcAudioFilePlayMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_STOP_PLAYBACK:
      handleAudioStopPlayback((const AcAudioStopPlaybackMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_PAUSE_PLAYBACK:
      handleAudioPausePlayback((const AcAudioPausePlaybackMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_RESUME_PLAYBACK:
      handleAudioResumePlayback((const AcAudioResumePlaybackMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_RECORD_START:
      handleAudioRecordStart((const AcAudioRecordStartMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_RECORD_STOP:
      handleAudioRecordStop((const AcAudioRecordStopMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_TONE_START:
      handleAudioToneStart((const AcAudioToneStartMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_AUDIO_TONE_STOP:
      handleAudioToneStop((const AcAudioToneStopMsg&)rRawMsg);
      return TRUE;
   default:
      break;
   }

   return FALSE;
}

UtlBoolean XCpAbstractCall::handleNotificationMessage(const AcNotificationMsg& rRawMsg)
{
   return FALSE;
}

UtlBoolean XCpAbstractCall::handleTimerMessage(const CpTimerMsg& rRawMsg)
{
   switch ((CpTimerMsg::SubTypeEnum)rRawMsg.getMsgSubType())
   {
   case CpTimerMsg::CP_TIMER_FIRST:
      // handle your message here
   default:
      break;
   }

   return FALSE;
}

OsStatus XCpAbstractCall::gainFocus()
{
#ifndef DISABLE_LOCAL_AUDIO
   CmGainFocusMsg gainFocusMsg(m_sId);
   return m_rCallManagerQueue.send(gainFocusMsg);
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpAbstractCall::yieldFocus()
{
#ifndef DISABLE_LOCAL_AUDIO
   CmYieldFocusMsg yieldFocusMsg(m_sId);
   return m_rCallManagerQueue.send(yieldFocusMsg);
#else
   return OS_SUCCESS;
#endif
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

OsStatus XCpAbstractCall::handleGainFocus(const AcGainFocusMsg& rMsg)
{
#ifndef DISABLE_LOCAL_AUDIO
   if (m_pMediaInterface && !m_bIsFocused)
   {
      OsStatus resFocus = m_pMediaInterface->giveFocus();
      if (resFocus == OS_SUCCESS)
      {
         m_bIsFocused = TRUE;
         return OS_SUCCESS;
      }
   }

   return OS_FAILED;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpAbstractCall::handleDefocus(const AcYieldFocusMsg& rMsg)
{
#ifndef DISABLE_LOCAL_AUDIO
   if (m_pMediaInterface && m_bIsFocused)
   {
      OsStatus resFocus = m_pMediaInterface->defocus();
      if (resFocus == OS_SUCCESS)
      {
         m_bIsFocused = FALSE;
         return OS_SUCCESS;
      }
   }

   return OS_FAILED;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpAbstractCall::handleAudioBufferPlay(const AcAudioBufferPlayMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->playBuffer(rMsg.getAudiobuf(), rMsg.getBufSize(),
         rMsg.getType(), rMsg.getRepeat(), rMsg.getLocal(), rMsg.getRemote(),
         rMsg.getMixWithMic(), rMsg.getDownScaling(), rMsg.getCookie());
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::handleAudioFilePlay(const AcAudioFilePlayMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->playAudio(rMsg.getAudioFile(), rMsg.getRepeat(), rMsg.getLocal(), rMsg.getRemote(),
         rMsg.getMixWithMic(), rMsg.getDownScaling(), rMsg.getCookie());
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::handleAudioPausePlayback(const AcAudioPausePlaybackMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->pausePlayback();
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::handleAudioResumePlayback(const AcAudioResumePlaybackMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->resumePlayback();
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::handleAudioStopPlayback(const AcAudioStopPlaybackMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->stopAudio();
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::handleAudioRecordStart(const AcAudioRecordStartMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->recordAudio(rMsg.getFile());
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::handleAudioRecordStop(const AcAudioRecordStopMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->stopRecording();
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::handleAudioToneStart(const AcAudioToneStartMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->startTone(rMsg.getToneId(), rMsg.getLocal(), rMsg.getRemote());
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::handleAudioToneStop(const AcAudioToneStopMsg& rMsg)
{
   if (m_pMediaInterface)
   {
      return m_pMediaInterface->stopTone();
   }

   return OS_FAILED;
}

UtlBoolean XCpAbstractCall::handlePhoneAppMessage(const OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;
   int msgSubType = rRawMsg.getMsgSubType();

   switch (msgSubType)
   {
   case SipMessage::NET_SIP_MESSAGE:
      return handleSipMessageEvent((const SipMessageEvent&)rRawMsg);
   default:
      {
         OsSysLog::add(FAC_CP, PRI_ERR, "Unknown PHONE_APP XCpAbstractCall message subtype: %d\n", msgSubType);
         break;
      }
   }

   return bResult;
}

UtlBoolean XCpAbstractCall::handleConnectionNotfMessage(const OsIntPtrMsg& rMsg)
{
   CpNotificationMsgMedia media = (CpNotificationMsgMedia)rMsg.getMsgSubType();
   CpNotificationMsgType type = (CpNotificationMsgType)rMsg.getData1();
   int mediaConnectionId = rMsg.getData2();
   intptr_t pData1 = rMsg.getData3();
   intptr_t pData2 = rMsg.getData4();

   switch(type)
   {
   case CP_NOTIFICATION_DTMF_INBAND:
      fireSipXMediaConnectionEvent(CP_MEDIA_REMOTE_DTMF, CP_MEDIA_CAUSE_DTMF_INBAND, (CP_MEDIA_TYPE)media, mediaConnectionId, pData1, pData2);
      break;
   case CP_NOTIFICATION_DTMF_RFC2833:
      fireSipXMediaConnectionEvent(CP_MEDIA_REMOTE_DTMF, CP_MEDIA_CAUSE_DTMF_RFC2833, (CP_MEDIA_TYPE)media, mediaConnectionId, pData1, pData2);
      break;
   case CP_NOTIFICATION_DTMF_SIPINFO:
      fireSipXMediaConnectionEvent(CP_MEDIA_REMOTE_DTMF, CP_MEDIA_CAUSE_DTMF_SIPINFO, (CP_MEDIA_TYPE)media, mediaConnectionId, pData1, pData2);
      break;
   default:
      assert(false);
   }

   return TRUE;
}

UtlBoolean XCpAbstractCall::handleInterfaceNotfMessage(const OsIntPtrMsg& rMsg)
{
   CpNotificationMsgMedia media = (CpNotificationMsgMedia)rMsg.getMsgSubType();
   CpNotificationMsgType type = (CpNotificationMsgType)rMsg.getData1();
   intptr_t pData1 = rMsg.getData2();
   intptr_t pData2 = rMsg.getData3();

   switch(type)
   {
   case CP_NOTIFICATION_START_PLAY_FILE:
      fireSipXMediaInterfaceEvent(CP_MEDIA_PLAYFILE_START, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_STOP_PLAY_FILE:
      fireSipXMediaInterfaceEvent(CP_MEDIA_PLAYFILE_STOP, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_START_PLAY_BUFFER:
      fireSipXMediaInterfaceEvent(CP_MEDIA_PLAYBUFFER_START, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_STOP_PLAY_BUFFER:
      fireSipXMediaInterfaceEvent(CP_MEDIA_PLAYBUFFER_STOP, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_PAUSE_PLAYBACK:
      fireSipXMediaInterfaceEvent(CP_MEDIA_PLAYBACK_PAUSED, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_RESUME_PLAYBACK:
      fireSipXMediaInterfaceEvent(CP_MEDIA_PLAYBACK_RESUMED, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_RECORDING_STARTED:
      fireSipXMediaInterfaceEvent(CP_MEDIA_RECORDING_START, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_RECORDING_STOPPED:
      fireSipXMediaInterfaceEvent(CP_MEDIA_RECORDING_STOP, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   default:
      assert(false);
   }

   return TRUE;
}

void XCpAbstractCall::releaseMediaInterface()
{
   if (m_pMediaInterface)
   {
      // lock is not needed
      m_pMediaInterface->release();
      m_pMediaInterface = NULL;
   }
}

CpMediaInterface* XCpAbstractCall::getMediaInterface() const
{
   return m_pMediaInterface;
}

OsStatus XCpAbstractCall::acquire(const OsTime& rTimeout /*= OsTime::OS_INFINITY*/)
{
   return m_instanceRWMutex.acquireRead();
}

OsStatus XCpAbstractCall::tryAcquire()
{
   return m_instanceRWMutex.tryAcquireRead();
}

OsStatus XCpAbstractCall::release()
{
   return m_instanceRWMutex.releaseRead();
}

/* ============================ FUNCTIONS ================================= */

