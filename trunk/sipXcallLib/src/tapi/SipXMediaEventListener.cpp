//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
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

SipXMediaEventListener::SipXMediaEventListener( SIPX_INST pInst )
   : m_pInst(pInst)
{

}

SipXMediaEventListener::~SipXMediaEventListener()
{

}

/* ============================ MANIPULATORS ============================== */

void SipXMediaEventListener::OnMediaLocalStart( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_LOCAL_START,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData);
}

void SipXMediaEventListener::OnMediaLocalStop( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_LOCAL_STOP,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData);
}

void SipXMediaEventListener::OnMediaRemoteStart( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_REMOTE_START,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData);
}

void SipXMediaEventListener::OnMediaRemoteStop( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_REMOTE_STOP,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData);
}

void SipXMediaEventListener::OnMediaRemoteSilent( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_REMOTE_SILENT,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData);
}

void SipXMediaEventListener::OnMediaPlayfileStart( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_PLAYFILE_START,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData,
                      event.m_pCookie,
                      event.m_playBufferIndex);
}

void SipXMediaEventListener::OnMediaPlayfileStop( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_PLAYFILE_STOP,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData,
                      event.m_pCookie,
                      event.m_playBufferIndex);
}

void SipXMediaEventListener::OnMediaPlaybufferStart( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_PLAYBUFFER_START,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData,
                      event.m_pCookie,
                      event.m_playBufferIndex);
}

void SipXMediaEventListener::OnMediaPlaybufferStop( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_PLAYBUFFER_STOP,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData,
                      event.m_pCookie,
                      event.m_playBufferIndex);
}

void SipXMediaEventListener::OnMediaPlaybackPaused( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
      event.m_sCallId,
      event.m_sSessionCallId,
      event.m_sRemoteAddress,
      MEDIA_PLAYBACK_PAUSED,
      event.m_Cause,
      event.m_MediaType,
      event.m_pEventData,
      event.m_pCookie,
      event.m_playBufferIndex);
}

void SipXMediaEventListener::OnMediaPlaybackResumed( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
      event.m_sCallId,
      event.m_sSessionCallId,
      event.m_sRemoteAddress,
      MEDIA_PLAYBACK_RESUMED,
      event.m_Cause,
      event.m_MediaType,
      event.m_pEventData,
      event.m_pCookie,
      event.m_playBufferIndex);
}

void SipXMediaEventListener::OnMediaRemoteDTMF( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_REMOTE_DTMF,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData);
}

void SipXMediaEventListener::OnMediaDeviceFailure( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_DEVICE_FAILURE,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData);
}

void SipXMediaEventListener::OnMediaRemoteActive( const CpMediaEvent& event )
{
   sipxFireMediaEvent(m_pInst,
                      event.m_sCallId,
                      event.m_sSessionCallId,
                      event.m_sRemoteAddress,
                      MEDIA_REMOTE_ACTIVE,
                      event.m_Cause,
                      event.m_MediaType,
                      event.m_pEventData);
}

void SipXMediaEventListener::OnMediaRecordingStart(const CpMediaEvent& event)
{
   sipxFireMediaEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_sRemoteAddress,
                     MEDIA_RECORDING_START,
                     event.m_Cause,
                     event.m_MediaType,
                     event.m_pEventData);
}

void SipXMediaEventListener::OnMediaRecordingStop(const CpMediaEvent& event)
{
   sipxFireMediaEvent(m_pInst,
                     event.m_sCallId,
                     event.m_sSessionCallId,
                     event.m_sRemoteAddress,
                     MEDIA_RECORDING_STOP,
                     event.m_Cause,
                     event.m_MediaType,
                     event.m_pEventData);
}
/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

