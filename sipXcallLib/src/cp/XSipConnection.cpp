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

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XSipConnection::XSipConnection(const UtlString& sAbstractCallId,
                               SipUserAgent& rSipUserAgent,
                               CpMediaInterfaceProvider* pMediaInterfaceProvider,
                               CpCallStateEventListener* pCallEventListener,
                               SipInfoStatusEventListener* pInfoStatusEventListener,
                               SipSecurityEventListener* pSecurityEventListener,
                               CpMediaEventListener* pMediaEventListener)
: m_instanceRWMutex(OsRWMutex::Q_FIFO)
, m_stateMachine(m_sipConnectionContext, rSipUserAgent, pMediaInterfaceProvider)
, m_rSipUserAgent(rSipUserAgent)
, m_pMediaInterfaceProvider(pMediaInterfaceProvider)
, m_pCallEventListener(pCallEventListener)
, m_pInfoStatusEventListener(pInfoStatusEventListener)
, m_pSecurityEventListener(pSecurityEventListener)
, m_pMediaEventListener(pMediaEventListener)
{
   m_sipConnectionContext.m_sAbstractCallId = sAbstractCallId;
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

int XSipConnection::getMediaConnectionId() const
{
   OsReadLock lock(m_sipConnectionContext);
   return m_sipConnectionContext.m_mediaConnectionId;
}

void XSipConnection::getAbstractCallId(UtlString& sAbstractCallId) const
{
   OsReadLock lock(m_sipConnectionContext);
   sAbstractCallId = m_sipConnectionContext.m_sAbstractCallId;
}

void XSipConnection::getRemoteAddress(UtlString& sRemoteAddress) const
{
   Url remoteUrl;
   {
      OsReadLock lock(m_sipConnectionContext);
      m_sipConnectionContext.m_sipDialog.getRemoteField(remoteUrl);
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

void XSipConnection::prepareMediaEvent(CpMediaEvent& event,
                                       CP_MEDIA_CAUSE cause,
                                       CP_MEDIA_TYPE type)
{
   Url remoteField;
   {
      OsReadLock lock(m_sipConnectionContext);
      event.m_sCallId = m_sipConnectionContext.m_sAbstractCallId; // copy id of abstract call
      m_sipConnectionContext.m_sipDialog.getCallId(event.m_sSessionCallId); // copy sip callid
      m_sipConnectionContext.m_sipDialog.getRemoteField(remoteField); // copy remote field (From or To) including tag
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
      OsReadLock lock(m_sipConnectionContext);
      event.m_sCallId = m_sipConnectionContext.m_sAbstractCallId; // copy id of abstract call
      event.m_pSipDialog = new SipDialog(m_sipConnectionContext.m_sipDialog); // assign copy of sip dialog. Gets deleted in destructor.
   }

   event.m_cause = eMinor;
   event.m_sOriginalSessionCallId = sOriginalSessionCallId;
   event.m_sipResponseCode = sipResponseCode;
   event.m_sResponseText = sResponseText;
}

void XSipConnection::fireSipXInfoStatusEvent(CP_INFOSTATUS_EVENT event,
                                             SIPXTACK_MESSAGE_STATUS status,
                                             const UtlString& sResponseText,
                                             int responseCode /*= 0*/)
{
   if (m_pInfoStatusEventListener)
   {
      SipInfoStatusEvent infoEvent;
      infoEvent.m_status = status;
      infoEvent.m_sResponseText = sResponseText;
      infoEvent.m_iResponseCode = responseCode;

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

/* ============================ FUNCTIONS ================================= */
