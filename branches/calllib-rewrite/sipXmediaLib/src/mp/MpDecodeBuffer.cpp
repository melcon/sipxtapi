//  
// Copyright (C) 2006-2007 SIPez LLC. 
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

#include "assert.h"
#include "string.h"

#include "os/OsDefs.h"
#include "mp/MpDecodeBuffer.h"
#include "mp/MpDecoderBase.h"
#include "mp/MpMisc.h"
#include <mp/MprDejitter.h>

static int debugCount = 0;

/* ============================ CREATORS ================================== */

MpDecodeBuffer::MpDecodeBuffer(MprDejitter* pDejitter, int samplesPerFrame, int samplesPerSec)
: m_pDejitter(pDejitter)
, m_samplesPerFrame(samplesPerFrame)
, m_samplesPerSec(samplesPerSec)
{
   for (int i=0; i<JbPayloadMapSize; i++)
      m_pDecoderMap[i] = NULL;

   memset(m_pDecoderList, 0, sizeof(m_pDecoderList));

#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   memset(m_pResamplerMap, 0, sizeof(m_pResamplerMap));
#endif

   m_decodeBufferCount = 0;
   m_decodeBufferIn = 0;
   m_decodeBufferOut = 0;

   debugCount = 0;
}

// Destructor
MpDecodeBuffer::~MpDecodeBuffer()
{
   destroyResamplers();
}

/* ============================ MANIPULATORS ============================== */

int MpDecodeBuffer::getSamples(MpAudioSample *samplesBuffer,
                               int requiredSamples) // required number of samples, for flowgraph sample rate
{
   if (m_pDejitter)
   {
      // first do actions that need to be done regardless of whether we have enough
      // samples
      m_pDejitter->frameIncrement();

      // now get more samples if we don't have enough of them
      if (m_decodeBufferCount < requiredSamples)
      {
         // we don't have enough samples. pull some from jitter buffer
         for (int i = 0; m_pDecoderList[i]; i++)
         {
            // loop through all decoders and pull frames for them
            int payloadType = m_pDecoderList[i]->getPayloadType();
            MpRtpBufPtr rtp = m_pDejitter->pullPacket(payloadType);

            while (rtp.isValid())
            {
               // if buffer is valid, then there is undecoded data, we decode it
               // until we have enough samples here or jitter buffer has no more data
               pushPacket(rtp);

               if (m_decodeBufferCount < requiredSamples)
               {
                  // still don't have enough, pull more data
                  rtp = m_pDejitter->pullPacket(payloadType);
               }
               else
               {
                  break;
                  // we cant break out of for loop, as we need to process rfc2833 too
               }
            }  
         }  
      }
   }
      
   // Check we have some available decoded data
   if (m_decodeBufferCount != 0)
   {
      // We could not return more then we have
      requiredSamples = min(requiredSamples, m_decodeBufferCount);

      memcpy(samplesBuffer, m_decodeBuffer + m_decodeBufferOut, requiredSamples * sizeof(MpAudioSample));

      m_decodeBufferCount -= requiredSamples;
      m_decodeBufferOut += requiredSamples;

      if (m_decodeBufferOut >= g_decodeBufferSize)
         m_decodeBufferOut -= g_decodeBufferSize;
   }

   return requiredSamples;
}


int MpDecodeBuffer::setCodecList(MpDecoderBase** decoderList, int decoderCount)
{
   memset(m_pDecoderList, 0, sizeof(m_pDecoderList));

   // For every payload type, load in a codec pointer, or a NULL if it isn't there
	for(int i = 0; (i < decoderCount) && (i < JbPayloadMapSize); i++)
   {
		int payloadType = decoderList[i]->getPayloadType();
   	m_pDecoderMap[payloadType] = decoderList[i];
      m_pDecoderList[i] = decoderList[i];
	}

   setupResamplers(decoderList, decoderCount);

   return 0;
}

int MpDecodeBuffer::pushPacket(MpRtpBufPtr &rtpPacket)
{
   unsigned int availableBufferSize;          // number of samples could be written to decoded buffer
   unsigned decodedSamples = 0; // number of samples, returned from decoder
   uint8_t payloadType;     // RTP packet payload type
   MpDecoderBase* decoder;  // decoder for the packet

   payloadType = rtpPacket->getRtpPayloadType();

   // Ignore illegal payload types
   if (payloadType >= JbPayloadMapSize)
      return 0;

   // Get decoder
   decoder = m_pDecoderMap[payloadType];
   if (decoder == NULL)
      return 0; // If we can't decode it, we must ignore it?

   // Calculate space available for decoded samples
   if (m_decodeBufferIn > m_decodeBufferOut || m_decodeBufferCount == 0)
   {
      availableBufferSize = g_decodeBufferSize-m_decodeBufferIn;
   }
   else
   {
      availableBufferSize = m_decodeBufferOut-m_decodeBufferIn;
   }

   // decode samples from decoder, and copy them into decode buffer
#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   if (decoder->getInfo()->getCodecType() != SdpCodec::SDP_CODEC_TONES &&
      decoder->getInfo()->getSamplingRate() != m_samplesPerSec)
   {
      // need to resample
      SpeexResamplerState* pResamplerState = m_pResamplerMap[payloadType];
      if (pResamplerState)
      {
         MpAudioBufPtr pTmpBuffer = NULL;
         pTmpBuffer = MpMisc.m_pRawAudioPool->getBuffer();
         if (!pTmpBuffer.isValid())
         {
            return 0;
         }
         unsigned int tmpAvailableBufferSize = MpMisc.m_audioSamplesPerFrame * sizeof(MpAudioSample); // in bytes
         unsigned int tmpDecodedSamples = decoder->decode(rtpPacket, tmpAvailableBufferSize, pTmpBuffer->getSamplesWritePtr());
         if (tmpDecodedSamples > 0)
         {
            // resample decoded samples
            speex_resampler_process_int(pResamplerState, 0,
               (spx_int16_t*)pTmpBuffer->getSamplesWritePtr(), &tmpDecodedSamples,
               (spx_int16_t*)(m_decodeBuffer+m_decodeBufferIn), &availableBufferSize);
            decodedSamples = availableBufferSize; // speex overwrites availableBufferSize with number of output samples
         }
      }
      else
      {
         // we can't resample, and can't decode
         assert(false);
      }
   }
   else
   {
      // no need to resample
      decodedSamples = decoder->decode(rtpPacket, availableBufferSize, m_decodeBuffer+m_decodeBufferIn);
   }
#else
   // no need to resample
   decodedSamples = decoder->decode(rtpPacket, availableBufferSize, m_decodeBuffer+m_decodeBufferIn);
#endif

   // Update buffer state
   m_decodeBufferCount += decodedSamples;
   m_decodeBufferIn += decodedSamples;
   // Reset write pointer if we reach end of buffer
   if (m_decodeBufferIn >= g_decodeBufferSize)
      m_decodeBufferIn = 0;

   return 0;
}

void MpDecodeBuffer::destroyResamplers()
{
#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   for (int i = 0; i < JbPayloadMapSize; i++)
   {
      if (m_pDecoderMap[i])
      {
         speex_resampler_destroy(m_pResamplerMap[i]);
         m_pDecoderMap[i] = NULL;
      }      
   }
#endif
}

void MpDecodeBuffer::setupResamplers(MpDecoderBase** decoderList, int decoderCount)
{
#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   destroyResamplers();

   for(int i = 0; (i < decoderCount) && (i < JbPayloadMapSize); i++)
   {
      int payloadType = decoderList[i]->getPayloadType();
      if (!m_pResamplerMap[payloadType])
      {
         int error;
         m_pResamplerMap[payloadType] = speex_resampler_init(1,
            decoderList[i]->getInfo()->getSamplingRate(), // resample from codec sample rate
            m_samplesPerSec, // into flowgraph sample rate
            SPEEX_RESAMPLER_QUALITY_VOIP, &error);
      }
   }
#endif
}
