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
#include <mi/CpMediaInterface.h>
#include <net/SipInfoEventListener.h>
#include <cp/CpMediaInterfaceProvider.h>
#include <cp/XSipConnection.h>
#include <cp/XSipConnectionContext.h>
#include <cp/CpMediaEventListener.h>
#include <cp/CpCallStateEventListener.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
const UtlContainableType XSipConnection::TYPE = "XSipConnection";

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

#ifdef _WIN32
#pragma warning(disable:4355)
#endif

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XSipConnection::XSipConnection(const UtlString& sAbstractCallId,
                               const SipDialog& sipDialog,
                               SipUserAgent& rSipUserAgent,
                               int inviteExpireSeconds,
                               CpMediaInterfaceProvider& rMediaInterfaceProvider,
                               CpMessageQueueProvider& rMessageQueueProvider,
                               const CpNatTraversalConfig& natTraversalConfig,
                               CpCallStateEventListener* pCallEventListener,
                               SipInfoStatusEventListener* pInfoStatusEventListener,
                               SipInfoEventListener* pInfoEventListener,
                               SipSecurityEventListener* pSecurityEventListener,
                               CpMediaEventListener* pMediaEventListener)
: m_instanceRWMutex(OsRWMutex::Q_FIFO)
, m_stateMachine(rSipUserAgent, rMediaInterfaceProvider, rMessageQueueProvider, *this, natTraversalConfig)
, m_rSipConnectionContext(m_stateMachine.getSipConnectionContext())
, m_rSipUserAgent(rSipUserAgent)
, m_rMediaInterfaceProvider(rMediaInterfaceProvider)
, m_rMessageQueueProvider(rMessageQueueProvider)
, m_pCallEventListener(pCallEventListener)
, m_pInfoStatusEventListener(pInfoStatusEventListener)
, m_pInfoEventListener(pInfoEventListener)
, m_pSecurityEventListener(pSecurityEventListener)
, m_pMediaEventListener(pMediaEventListener)
, m_natTraversalConfig(natTraversalConfig)
{
   m_rSipConnectionContext.m_sAbstractCallId = sAbstractCallId;
   m_rSipConnectionContext.m_sipDialog = sipDialog;
   m_rSipConnectionContext.m_defaultSessionExpiration = inviteExpireSeconds;
   m_stateMachine.setStateObserver(this); // register for state machine state change notifications
}

XSipConnection::~XSipConnection()
{
   m_stateMachine.setStateObserver(NULL);
   fireSipXCallEvent(CP_CALLSTATE_DESTROYED, CP_CALLSTATE_CAUSE_NORMAL);
}

/* ============================ MANIPULATORS ============================== */

OsStatus XSipConnection::acquireExclusive()
{
   return m_instanceRWMutex.acquireWrite();
}

void XSipConnection::handleSipXMediaEvent(CP_MEDIA_EVENT event,
                                          CP_MEDIA_CAUSE cause,
                                          CP_MEDIA_TYPE type,
                                          intptr_t pEventData1 /*= 0*/,
                                          intptr_t pEventData2 /*= 0*/)
{
   fireSipXMediaEvent(event, cause, type, pEventData1, pEventData2);
}

void XSipConnection::handleSipXCallEvent(CP_CALLSTATE_EVENT eventCode,
                                         CP_CALLSTATE_CAUSE causeCode,
                                         const UtlString& sOriginalSessionCallId /*= NULL*/,
                                         int sipResponseCode /*= 0*/,
                                         const UtlString& sResponseText /*= NULL*/)
{
   fireSipXCallEvent(eventCode, causeCode, sOriginalSessionCallId, sipResponseCode, sResponseText);
}

OsStatus XSipConnection::connect(const UtlString& sipCallId,
                                 const UtlString& localTag,
                                 const UtlString& toAddress,
                                 const UtlString& fromAddress,
                                 const UtlString& locationHeader,
                                 CP_CONTACT_ID contactId)
{
   return m_stateMachine.connect(sipCallId, localTag, toAddress, fromAddress, locationHeader, contactId);
}

OsStatus XSipConnection::acceptConnection(const UtlString& locationHeader,
                                          CP_CONTACT_ID contactId)
{
   return OS_FAILED;
}

OsStatus XSipConnection::rejectConnection()
{
   return OS_FAILED;
}

OsStatus XSipConnection::redirectConnection(const UtlString& sRedirectSipUrl)
{
   return OS_FAILED;
}

OsStatus XSipConnection::answerConnection()
{
   return OS_FAILED;
}

OsStatus XSipConnection::dropConnection()
{
   return m_stateMachine.dropConnection();
}

OsStatus XSipConnection::transferBlind(const UtlString& sTransferSipUrl)
{
   return OS_FAILED;
}

OsStatus XSipConnection::holdConnection()
{
   return m_stateMachine.holdConnection();
}

OsStatus XSipConnection::unholdConnection()
{
   return m_stateMachine.unholdConnection();
}

OsStatus XSipConnection::muteInputConnection()
{
   int mediaConnectionId = getMediaConnectionId();
   CpMediaInterface *pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface(FALSE);
   if (pMediaInterface)
   {
      return pMediaInterface->muteInput(mediaConnectionId, TRUE);
   }

   return OS_FAILED;
}

OsStatus XSipConnection::unmuteInputConnection()
{
   int mediaConnectionId = getMediaConnectionId();
   CpMediaInterface *pMediaInterface = m_rMediaInterfaceProvider.getMediaInterface(FALSE);
   if (pMediaInterface)
   {
      return pMediaInterface->muteInput(mediaConnectionId, FALSE);
   }

   return OS_FAILED;
}

OsStatus XSipConnection::renegotiateCodecsConnection()
{
   return m_stateMachine.renegotiateCodecsConnection();
}

OsStatus XSipConnection::sendInfo(const UtlString& sContentType,
                                  const char* pContent,
                                  const size_t nContentLength,
                                  void* pCookie)
{
   return m_stateMachine.sendInfo(sContentType, pContent, nContentLength, pCookie);
}

UtlBoolean XSipConnection::handleTimerMessage(const ScTimerMsg& timerMsg)
{
   return m_stateMachine.handleTimerMessage(timerMsg);
}

UtlBoolean XSipConnection::handleSipMessageEvent(const SipMessageEvent& rSipMsgEvent)
{
   return m_stateMachine.handleSipMessageEvent(rSipMsgEvent);
}

UtlBoolean XSipConnection::handleCommandMessage(const ScCommandMsg& rMsg)
{
   return m_stateMachine.handleCommandMessage(rMsg);
}

UtlBoolean XSipConnection::handleNotificationMessage(const ScNotificationMsg& rMsg)
{
   return m_stateMachine.handleNotificationMessage(rMsg);
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
   OsReadLock lock(m_rSipConnectionContext);
   sSipDialog = m_rSipConnectionContext.m_sipDialog;
}

void XSipConnection::getSipCallId(UtlString& sSipCallId) const
{
   OsReadLock lock(m_rSipConnectionContext);
   m_rSipConnectionContext.m_sipDialog.getCallId(sSipCallId);
}

void XSipConnection::getRemoteUserAgent(UtlString& sRemoteUserAgent) const
{
   OsReadLock lock(m_rSipConnectionContext);
   sRemoteUserAgent = m_rSipConnectionContext.m_remoteUserAgent;
}

int XSipConnection::getMediaEventConnectionId() const
{
   // no need to lock atomic
   return m_rSipConnectionContext.m_mediaEventConnectionId;
}

int XSipConnection::getMediaConnectionId() const
{
   // no need to lock atomic
   return m_rSipConnectionContext.m_mediaConnectionId;
}

void XSipConnection::getAbstractCallId(UtlString& sAbstractCallId) const
{
   OsReadLock lock(m_rSipConnectionContext);
   sAbstractCallId = m_rSipConnectionContext.m_sAbstractCallId;
}

void XSipConnection::getRemoteAddress(UtlString& sRemoteAddress) const
{
   Url remoteUrl;
   {
      OsReadLock lock(m_rSipConnectionContext);
      m_rSipConnectionContext.m_sipDialog.getRemoteField(remoteUrl);
   }
   remoteUrl.toString(sRemoteAddress);
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
   OsReadLock lock(m_rSipConnectionContext);
   return m_rSipConnectionContext.m_sipDialog.compareDialogs(sSipDialog);
}

SipConnectionStateContext::MediaSessionState XSipConnection::getMediaSessionState() const
{
   return m_stateMachine.getMediaSessionState();
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void XSipConnection::handleStateEntry(ISipConnectionState::StateEnum state)
{

}

void XSipConnection::handleStateExit(ISipConnectionState::StateEnum state)
{

}

void XSipConnection::prepareMediaEvent(CpMediaEvent& event,
                                       CP_MEDIA_CAUSE cause,
                                       CP_MEDIA_TYPE type)
{
   Url remoteField;
   {
      OsReadLock lock(m_rSipConnectionContext);
      event.m_sCallId = m_rSipConnectionContext.m_sAbstractCallId; // copy id of abstract call
      m_rSipConnectionContext.m_sipDialog.getCallId(event.m_sSessionCallId); // copy sip callid
      m_rSipConnectionContext.m_sipDialog.getRemoteField(remoteField); // copy remote field (From or To) including tag
   }
   remoteField.toString(event.m_sRemoteAddress);

   event.m_cause = cause;
   event.m_mediaType = type;
}

void XSipConnection::prepareCallStateEvent(CpCallStateEvent& event,
                                           CP_CALLSTATE_CAUSE eMinor,
                                           const UtlString& sOriginalSessionCallId /*= NULL*/,
                                           int sipResponseCode /*= 0*/,
                                           const UtlString& sResponseText /*= NULL*/)
{
   {
      OsReadLock lock(m_rSipConnectionContext);
      event.m_sCallId = m_rSipConnectionContext.m_sAbstractCallId; // copy id of abstract call
      event.m_pSipDialog = new SipDialog(m_rSipConnectionContext.m_sipDialog); // assign copy of sip dialog. Gets deleted in destructor.
   }

   event.m_cause = eMinor;
   event.m_sOriginalSessionCallId = sOriginalSessionCallId;
   event.m_sipResponseCode = sipResponseCode;
   event.m_sResponseText = sResponseText;
}

void XSipConnection::fireSipXInfoStatusEvent(CP_INFOSTATUS_EVENT event,
                                             SIPXTACK_MESSAGE_STATUS status,
                                             const UtlString& sResponseText,
                                             int responseCode,
                                             void* pCookie)
{
   if (m_pInfoStatusEventListener)
   {
      UtlString sAbstractCallId;
      getAbstractCallId(sAbstractCallId);
      SipInfoStatusEvent infoEvent(sAbstractCallId, status, responseCode, sResponseText, pCookie);

      switch(event)
      {
      case CP_INFOSTATUS_RESPONSE:
         m_pInfoStatusEventListener->OnResponse(infoEvent);
         break;
      case CP_INFOSTATUS_NETWORK_ERROR:
         m_pInfoStatusEventListener->OnNetworkError(infoEvent);
         break;
      default:
         ;
      }
   }
}

void XSipConnection::fireSipXInfoEvent(const UtlString& sContentType,
                                       const char* pContent /*= NULL*/,
                                       size_t nContentLength /*= 0*/)
{
   if (m_pInfoEventListener)
   {
      UtlString sAbstractCallId;
      getAbstractCallId(sAbstractCallId);
      SipInfoEvent infoEvent(sAbstractCallId, sContentType, pContent, nContentLength);

      m_pInfoEventListener->OnInfoMessage(infoEvent);
   }
}

void XSipConnection::fireSipXSecurityEvent(SIPXTACK_SECURITY_EVENT event,
                                           SIPXTACK_SECURITY_CAUSE cause,
                                           const UtlString& sSRTPkey,
                                           void* pCertificate,
                                           size_t nCertificateSize,
                                           const UtlString& sSubjAltName,
                                           const UtlString& sSessionCallId,
                                           const UtlString& sRemoteAddress)
{
   if (m_pSecurityEventListener)
   {
      SipSecurityEvent secEvent;
      secEvent.m_event = event;
      secEvent.m_cause = cause;
      secEvent.m_sSRTPkey = sSRTPkey;
      secEvent.m_pCertificate = pCertificate;
      secEvent.m_nCertificateSize = nCertificateSize;
      secEvent.m_sSubjAltName = sSubjAltName;
      secEvent.m_SessionCallId = sSessionCallId;
      secEvent.m_sRemoteAddress = sRemoteAddress;

      switch(event)
      {
      case SIPXTACK_SECURITY_ENCRYPT:
         m_pSecurityEventListener->OnEncrypt(secEvent);
         break;
      case SIPXTACK_SECURITY_DECRYPT:
         m_pSecurityEventListener->OnDecrypt(secEvent);
         break;
      case SIPXTACK_SECURITY_TLS:
         m_pSecurityEventListener->OnTLS(secEvent);
         break;
      default:
         ;
      }

      secEvent.m_pCertificate = NULL; // must be zeroed before ~SipSecurityEvent runs
   }
}

void XSipConnection::fireSipXMediaEvent(CP_MEDIA_EVENT event,
                                        CP_MEDIA_CAUSE cause,
                                        CP_MEDIA_TYPE type,
                                        intptr_t pEventData1 /*= 0*/,
                                        intptr_t pEventData2 /*= 0*/)
{
   if (m_pMediaEventListener)
   {
      CpMediaEvent mediaEvent;
      prepareMediaEvent(mediaEvent, cause, type);

      switch(event)
      {
      case CP_MEDIA_LOCAL_START:
         if (pEventData1)
         {
            mediaEvent.m_codec = *(CpCodecInfo*)pEventData1;
         }
         m_pMediaEventListener->OnMediaLocalStart(mediaEvent);
         break;
      case CP_MEDIA_LOCAL_STOP:
         if (pEventData1)
         {
            mediaEvent.m_codec = *(CpCodecInfo*)pEventData1;
         }
         m_pMediaEventListener->OnMediaLocalStop(mediaEvent);
         break;
      case CP_MEDIA_REMOTE_START:
         m_pMediaEventListener->OnMediaRemoteStart(mediaEvent);
         break;
      case CP_MEDIA_REMOTE_STOP:
         m_pMediaEventListener->OnMediaRemoteStop(mediaEvent);
         break;
      case CP_MEDIA_REMOTE_SILENT:
         mediaEvent.m_idleTime = (int)pEventData1;
         m_pMediaEventListener->OnMediaRemoteSilent(mediaEvent);
         break;
      case CP_MEDIA_PLAYFILE_START:
         mediaEvent.m_pCookie = (void*)pEventData1;
         mediaEvent.m_playBufferIndex = pEventData2;
         m_pMediaEventListener->OnMediaPlayfileStart(mediaEvent);
         break;
      case CP_MEDIA_PLAYFILE_STOP:
         mediaEvent.m_pCookie = (void*)pEventData1;
         mediaEvent.m_playBufferIndex = pEventData2;
         m_pMediaEventListener->OnMediaPlayfileStop(mediaEvent);
         break;
      case CP_MEDIA_PLAYBUFFER_START:
         mediaEvent.m_pCookie = (void*)pEventData1;
         mediaEvent.m_playBufferIndex = pEventData2;
         m_pMediaEventListener->OnMediaPlaybufferStart(mediaEvent);
         break;
      case CP_MEDIA_PLAYBUFFER_STOP:
         mediaEvent.m_pCookie = (void*)pEventData1;
         mediaEvent.m_playBufferIndex = pEventData2;
         m_pMediaEventListener->OnMediaPlaybufferStop(mediaEvent);
         break;
      case CP_MEDIA_PLAYBACK_PAUSED:
         mediaEvent.m_pCookie = (void*)pEventData1;
         mediaEvent.m_playBufferIndex = (int)pEventData2;
         m_pMediaEventListener->OnMediaPlaybackPaused(mediaEvent);
         break;
      case CP_MEDIA_PLAYBACK_RESUMED:
         mediaEvent.m_pCookie = (void*)pEventData1;
         mediaEvent.m_playBufferIndex = (int)pEventData2;
         m_pMediaEventListener->OnMediaPlaybackResumed(mediaEvent);
         break;
      case CP_MEDIA_REMOTE_DTMF:
         mediaEvent.m_toneId = (CP_TONE_ID)pEventData1;
         m_pMediaEventListener->OnMediaRemoteDTMF(mediaEvent);
         break;
      case CP_MEDIA_DEVICE_FAILURE:
         m_pMediaEventListener->OnMediaDeviceFailure(mediaEvent);
         break;
      case CP_MEDIA_REMOTE_ACTIVE:
         m_pMediaEventListener->OnMediaRemoteActive(mediaEvent);
         break;
      case CP_MEDIA_RECORDING_START:
         m_pMediaEventListener->OnMediaRecordingStart(mediaEvent);
         break;
      case CP_MEDIA_RECORDING_STOP:
         m_pMediaEventListener->OnMediaRecordingStop(mediaEvent);
         break;
      default:
         ;
      }
   }
}

void XSipConnection::fireSipXCallEvent(CP_CALLSTATE_EVENT eventCode,
                                       CP_CALLSTATE_CAUSE causeCode,
                                       const UtlString& sOriginalSessionCallId /*= NULL*/,
                                       int sipResponseCode /*= 0*/,
                                       const UtlString& sResponseText /*= NULL*/)
{
   if (m_pCallEventListener)
   {
      CpCallStateEvent event;
      prepareCallStateEvent(event, causeCode, sOriginalSessionCallId, sipResponseCode, sResponseText);

      switch(eventCode)
      {
      case CP_CALLSTATE_NEWCALL:
         m_pCallEventListener->OnNewCall(event);
         break;
      case CP_CALLSTATE_DIALTONE:
         m_pCallEventListener->OnDialTone(event);
         break;
      case CP_CALLSTATE_REMOTE_OFFERING:
         m_pCallEventListener->OnRemoteOffering(event);
         break;
      case CP_CALLSTATE_REMOTE_ALERTING:
         m_pCallEventListener->OnRemoteAlerting(event);
         break;
      case CP_CALLSTATE_CONNECTED:
         m_pCallEventListener->OnConnected(event);
         break;
      case CP_CALLSTATE_BRIDGED:
         m_pCallEventListener->OnBridged(event);
         break;
      case CP_CALLSTATE_HELD:
         m_pCallEventListener->OnHeld(event);
         break;
      case CP_CALLSTATE_REMOTE_HELD:
         m_pCallEventListener->OnRemoteHeld(event);
         break;
      case CP_CALLSTATE_DISCONNECTED:
         m_pCallEventListener->OnDisconnected(event);
         break;
      case CP_CALLSTATE_OFFERING:  
         m_pCallEventListener->OnOffering(event);
         break;
      case CP_CALLSTATE_ALERTING:
         m_pCallEventListener->OnAlerting(event);
         break;
      case CP_CALLSTATE_DESTROYED:
         m_pCallEventListener->OnDestroyed(event);
         break;
      case CP_CALLSTATE_TRANSFER_EVENT:
         m_pCallEventListener->OnTransferEvent(event);
         break;
      default:
         ;
      }
   }
}

OsStatus XSipConnection::acquire(const OsTime& rTimeout /*= OsTime::OS_INFINITY*/)
{
   return m_instanceRWMutex.acquireRead();
}

OsStatus XSipConnection::tryAcquire()
{
   return m_instanceRWMutex.tryAcquireRead();
}

OsStatus XSipConnection::release()
{
   return m_instanceRWMutex.releaseRead();
}

/* ============================ FUNCTIONS ================================= */
