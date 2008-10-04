//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/SipXMediaEventListener.h"
#include "tapi/SipXEvents.h"

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

SipXMediaEventListener::SipXMediaEventListener(SIPX_INST pInst)
   : m_pInst(pInst)
{

}

SipXMediaEventListener::~SipXMediaEventListener()
{

}

/* ============================ MANIPULATORS ============================== */

void SipXMediaEventListener::OnMediaLocalStart(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_LOCAL_START);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaLocalStop(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_LOCAL_STOP);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaRemoteStart(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_REMOTE_START);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaRemoteStop(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_REMOTE_STOP);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaRemoteSilent(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_REMOTE_SILENT);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaPlayfileStart(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_PLAYFILE_START);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaPlayfileStop(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_PLAYFILE_STOP);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaPlaybufferStart(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_PLAYBUFFER_START);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaPlaybufferStop(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_PLAYBUFFER_STOP);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaPlaybackPaused(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_PLAYBACK_PAUSED);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaPlaybackResumed(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_PLAYBACK_RESUMED);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaRemoteDTMF(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_REMOTE_DTMF);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaDeviceFailure(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_DEVICE_FAILURE);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaRemoteActive(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_REMOTE_ACTIVE);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaRecordingStart(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_RECORDING_START);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}

void SipXMediaEventListener::OnMediaRecordingStop(const CpMediaEvent& event)
{
   SipXMediaEvent eventPayload(event, MEDIA_RECORDING_STOP);
   sipxFireMediaEvent(m_pInst,
                      eventPayload);
}
/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


SIPX_CODEC_INFO getSipXCodecInfo(const CpCodecInfo& codecInfo)
{
   SIPX_CODEC_INFO sipxCodecInfo;
   memset(&sipxCodecInfo, 0, sizeof(SIPX_CODEC_INFO));

   sipxCodecInfo.bIsEncrypted = codecInfo.m_bIsEncrypted;
   SAFE_STRNCPY(sipxCodecInfo.audioCodec.cName, codecInfo.m_audioCodec.m_codecName.data(), SIPXTAPI_CODEC_NAMELEN);
   sipxCodecInfo.audioCodec.iBandWidth = (SIPX_AUDIO_BANDWIDTH_ID)codecInfo.m_audioCodec.m_iBandWidth;
   sipxCodecInfo.audioCodec.iPayloadType = codecInfo.m_audioCodec.m_iPayloadType;

   SAFE_STRNCPY(sipxCodecInfo.videoCodec.cName, codecInfo.m_videoCodec.m_codecName.data(), SIPXTAPI_CODEC_NAMELEN);
   sipxCodecInfo.videoCodec.iBandWidth = (SIPX_VIDEO_BANDWIDTH_ID)codecInfo.m_videoCodec.m_iBandWidth;
   sipxCodecInfo.videoCodec.iPayloadType = codecInfo.m_videoCodec.m_iPayloadType;

   return sipxCodecInfo;
}
