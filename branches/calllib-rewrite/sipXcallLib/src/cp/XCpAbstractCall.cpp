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
#include <net/SipMessage.h>
#include <mi/CpMediaInterfaceFactory.h>
#include <mi/CpMediaInterface.h>
#include <cp/XCpAbstractCall.h>
#include <cp/XSipConnection.h>
#include <cp/CpMessageTypes.h>
#include <cp/msg/AcCommandMsg.h>
#include <cp/msg/AcNotificationMsg.h>
#include <cp/msg/CmGainFocusMsg.h>
#include <cp/msg/CmYieldFocusMsg.h>

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
, m_mediaInterfaceRWMutex(OsRWMutex::Q_FIFO)
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
   if (m_pMediaInterface)
   {
      // lock is not needed
      m_pMediaInterface->release();
      m_pMediaInterface = NULL;
   }
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
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->startTone(iToneId, bLocal, bRemote);
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioToneStop()
{
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->stopTone();
   }

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
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->playAudio(audioFile, bRepeat, bLocal, bRemote,
         bMixWithMic, iDownScaling, pCookie);
   }

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
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->playBuffer((char*)pAudiobuf, (unsigned long)iBufSize, iType, bRepeat, bLocal, bRemote,
         FALSE, 100, pCookie);
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioStop()
{
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->stopAudio();
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::pauseAudioPlayback()
{
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->pausePlayback();
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::resumeAudioPlayback()
{
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->resumePlayback();
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioRecordStart(const UtlString& sFile)
{
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->recordAudio(sFile);
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioRecordStop()
{
   OsReadLock lock(m_mediaInterfaceRWMutex);

   if (m_pMediaInterface)
   {
      return m_pMediaInterface->stopRecording();
   }

   return OS_FAILED;
}

OsStatus XCpAbstractCall::acquire(const OsTime& rTimeout /*= OsTime::OS_INFINITY*/)
{
   return m_instanceRWMutex.acquireRead();
}

OsStatus XCpAbstractCall::acquireExclusive()
{
   return m_instanceRWMutex.acquireWrite();
}

OsStatus XCpAbstractCall::tryAcquire()
{
   return m_instanceRWMutex.tryAcquireRead();
}

OsStatus XCpAbstractCall::release()
{
   return m_instanceRWMutex.releaseRead();
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
      ptrLock->getMediaConnectionId(mediaConnID);
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
   switch ((AcCommandMsg::SubTypesEnum)rRawMsg.getMsgSubType())
   {
   case AcCommandMsg::AC_GAIN_FOCUS:
      handleGainFocus((const AcGainFocusMsg&)rRawMsg);
      return TRUE;
   case AcCommandMsg::AC_YIELD_FOCUS:
      handleDefocus((const AcYieldFocusMsg&)rRawMsg);
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
   OsReadLock lock(m_mediaInterfaceRWMutex);

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
   OsReadLock lock(m_mediaInterfaceRWMutex);

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

/* ============================ FUNCTIONS ================================= */

