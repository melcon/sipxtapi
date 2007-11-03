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
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <stdio.h> 
#include <assert.h> 
#include <string.h>

// APPLICATION INCLUDES
#include "os/OsMsgQ.h"
#include "mp/MpTypes.h"
#include "mp/MpCodec.h"
#include "mp/dmaTask.h"
#include "mp/MpBuf.h"
#include "mp/MpAudioBuf.h"
#include "mp/MpRtpBuf.h"
#include "mp/MpUdpBuf.h"
#include "mp/MpBufferMsg.h"
#include "mp/MpMisc.h"
#include "mp/NetInTask.h"
#include "mp/MprFromMic.h"
#include "mp/MprToSpkr.h"
#include "mp/MpMediaTask.h"
#include "mp/MpAudioDriverManager.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS

#define ECHO_BUFFER_Q_LEN 5
#define DEFAULT_INITIAL_AUDIO_BUFFERS 256

#define RTP_BUFS 250
#define RTCP_BUFS 16
#define UDP_BUFS 10

// STATIC VARIABLE INITIALIZATIONS
struct MpGlobals MpMisc;

// audio buffer grows if there aren't enough buffers
OsStatus mpStartUp(int sampleRate,
                   int samplesPerFrame)
{
   if (samplesPerFrame < 8 || sampleRate < 1)
   {
      return OS_FAILED;
   }
   
   MpMisc.m_audioSampleRate = sampleRate;
   MpMisc.m_audioSampleSize = sizeof(MpAudioSample);
   MpMisc.m_audioSamplesPerFrame = samplesPerFrame;
   MpMisc.m_audioFrameBytes = MpMisc.m_audioSampleSize * MpMisc.m_audioSamplesPerFrame;

   // Create buffer for audio data in mediagraph
   MpMisc.m_pRawAudioPool = new MpBufPool(samplesPerFrame*sizeof(MpAudioSample) + MpArrayBuf::getHeaderSize(),
                                          DEFAULT_INITIAL_AUDIO_BUFFERS);

   /*
   * Go get a buffer and fill with silence.  We will use this for muting
   * either or both of input and output, and whenever we are starved for
   * audio data.
   */
   MpAudioBufPtr sb = MpMisc.m_pRawAudioPool->getBuffer();

   if (!sb.isValid())
   {
      return OS_FAILED;
   }

   sb->setSamplesNumber(samplesPerFrame);
   memset(sb->getSamplesWritePtr(), 0, sb->getSamplesNumber()*sizeof(MpAudioSample));
   sb->setSpeechType(MpAudioBuf::MP_SPEECH_SILENT);
   MpMisc.m_fgSilence = sb;

   /*
   * generate a buffer called comfort noise buffer.
   *
   * TODO: generate real comfort noise, not just zeroes...
   */
   MpAudioBufPtr cnb = MpMisc.m_pRawAudioPool->getBuffer();

   if (!cnb.isValid())
   {
      return OS_FAILED;
   }

   cnb->setSamplesNumber(samplesPerFrame);
   memset(cnb->getSamplesWritePtr(), 0, cnb->getSamplesNumber() * sizeof(MpAudioSample));
   cnb->setSpeechType(MpAudioBuf::MP_SPEECH_COMFORT_NOISE);
   MpMisc.m_comfortNoise = cnb;

   // Create buffer for RTP packets
   MpMisc.m_pRtpPool = new MpBufPool(RTP_MTU + MpArrayBuf::getHeaderSize(), RTP_BUFS);

   // Create buffer for RTCP packets
   MpMisc.m_pRtcpPool = new MpBufPool(RTCP_MTU + MpArrayBuf::getHeaderSize(), RTCP_BUFS);

   // Create buffer for RTP and RTCP headers
   MpMisc.m_pRtpHeadersPool = new MpBufPool(sizeof(MpRtpBuf),
                    MpMisc.m_pRtpPool->getNumBlocks() + MpMisc.m_pRtcpPool->getNumBlocks());

   MpRtpBuf::smpDefaultPool = MpMisc.m_pRtpHeadersPool;

   // Create buffer for UDP packets
   MpMisc.m_pUdpPool = new MpBufPool(UDP_MTU + MpArrayBuf::getHeaderSize(), UDP_BUFS);

   // Create buffer for UDP packet headers
   MpMisc.m_pUdpHeadersPool = new MpBufPool(sizeof(MpUdpBuf), MpMisc.m_pUdpPool->getNumBlocks());

   // create queue for echo canceller
   MpMisc.m_pEchoQ = new OsMsgQ(ECHO_BUFFER_Q_LEN);

#ifndef DISABLE_LOCAL_AUDIO
   // create audio driver manager, which creates audio driver
   if (!MpAudioDriverManager::getInstance(TRUE))
   {
      return OS_FAILED;
   }
#endif

   // start media task
   if (!MpMediaTask::getMediaTask())
   {
      return OS_TASK_NOT_STARTED;
   }

   // start netintask
   if (startNetInTask() != OS_SUCCESS)
   {
      return OS_TASK_NOT_STARTED;
   }

   return OS_SUCCESS;
}

OsStatus mpShutdown(void)
{
   if (shutdownNetInTask() != OS_SUCCESS)
   {
      return OS_FAILED;
   }
  
   if (MpMediaTask::getMediaTask(FALSE))
   {
      // This will MpMediaTask::spInstance to NULL
      delete MpMediaTask::getMediaTask(FALSE);
   }

#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance(FALSE);
   if (pAudioManager)
   {
      pAudioManager->release();
   }
#endif

   if (MpMisc.m_pEchoQ)
   {
      delete MpMisc.m_pEchoQ;
      MpMisc.m_pEchoQ = NULL;
   }

   MpMisc.m_fgSilence.release();
   MpMisc.m_comfortNoise.release();

   if (MpMisc.m_pUdpHeadersPool)
   {
      delete MpMisc.m_pUdpHeadersPool;
      MpMisc.m_pUdpHeadersPool = NULL;
   }

   if (MpMisc.m_pUdpPool)
   {
      delete MpMisc.m_pUdpPool;
      MpMisc.m_pUdpPool = NULL;
   }

   if (MpMisc.m_pRtpHeadersPool)
   {
      delete MpMisc.m_pRtpHeadersPool;
      MpMisc.m_pRtpHeadersPool = NULL;
      MpRtpBuf::smpDefaultPool = NULL;
   }

   if (MpMisc.m_pRtpPool)
   {
      delete MpMisc.m_pRtpPool;
      MpMisc.m_pRtpPool = NULL;
   }

   if (MpMisc.m_pRtcpPool)
   {
      delete MpMisc.m_pRtcpPool;
      MpMisc.m_pRtcpPool = NULL;
   }

   if (MpMisc.m_pRawAudioPool)
   {
      delete MpMisc.m_pRawAudioPool;
      MpMisc.m_pRawAudioPool = NULL;
   }

   return OS_SUCCESS;
}

