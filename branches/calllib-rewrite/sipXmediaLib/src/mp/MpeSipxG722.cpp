//  
// Copyright (C) 2006 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//  
// Copyright (C) 2006 SIPfoundry Inc. 
// Licensed by SIPfoundry under the LGPL license. 
//  
// $$ 
////////////////////////////////////////////////////////////////////////////// 

#ifdef HAVE_SPAN_DSP /* [ */

#include "assert.h"
// APPLICATION INCLUDES
#include "mp/MpeSipxG722.h"
#include "mp/NetInTask.h"  

const MpCodecInfo MpeSipxG722::ms_codecInfo64(
   SdpCodec::SDP_CODEC_G722,    // codecType
   "",                 // codecVersion
   16000,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   64000,                      // bitRate. It doesn't matter right now.
   160*8,                       // minPacketBits
   160*8,                       // maxPacketBits
   320);                       // numSamplesPerFrame - 10ms frame

MpeSipxG722::MpeSipxG722(int payloadType)
: MpEncoderBase(payloadType, getCodecInfo())
{
}

MpeSipxG722::~MpeSipxG722()
{
}

OsStatus MpeSipxG722::initEncode(void)
{
   m_pG722state = g722_encode_init(NULL, getInfo()->getBitRate(), 0);

   if (m_pG722state)
   {
      return OS_SUCCESS;
   }
   else
   {
      return OS_FAILED;
   }
}

OsStatus MpeSipxG722::freeEncode(void)
{
   int res = g722_encode_release(m_pG722state);

   if (res == 0)
   {
      return OS_SUCCESS;
   }
   else
   {
      return OS_FAILED;
   }
}

OsStatus MpeSipxG722::encode(const MpAudioSample* pAudioSamples,
                            const int numSamples,
                            int& rSamplesConsumed,
                            unsigned char* pCodeBuf,
                            const int bytesLeft,
                            int& rSizeInBytes,
                            UtlBoolean& sendNow,
                            MpSpeechType& rAudioCategory)
{
   assert(numSamples == 160); // we expect 10ms frames (16Khz) - 160 samples

   rSizeInBytes = g722_encode(m_pG722state, pCodeBuf, pAudioSamples, numSamples);

   rSamplesConsumed = numSamples;
   rAudioCategory = MP_SPEECH_UNKNOWN;

   return OS_SUCCESS;
}

const MpCodecInfo* MpeSipxG722::getCodecInfo()
{
   return &ms_codecInfo64;
}

#endif /* HAVE_SPAN_DSP ] */
