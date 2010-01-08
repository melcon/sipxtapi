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
#include "mp/MpeSipxG726.h"
#include "mp/NetInTask.h"  

const MpCodecInfo MpeSipxG726::ms_codecInfo16(
   SdpCodec::SDP_CODEC_G726_16,    // codecType
   "G726-16",                 // codecVersion
   false,                      // usesNetEq
   8000,                       // samplingRate
   8,                          // numBitsPerSample
   1,                          // numChannels
   80,                        // interleaveBlockSize
   16000,                      // bitRate. It doesn't matter right now.
   20*8,                       // minPacketBits
   20*8,                       // avgPacketBits
   20*8,                       // maxPacketBits
   80);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxG726::ms_codecInfo24(
   SdpCodec::SDP_CODEC_G726_24,    // codecType
   "G726-24",                 // codecVersion
   false,                      // usesNetEq
   8000,                       // samplingRate
   8,                          // numBitsPerSample
   1,                          // numChannels
   80,                        // interleaveBlockSize
   24000,                      // bitRate. It doesn't matter right now.
   30*8,                       // minPacketBits
   30*8,                       // avgPacketBits
   30*8,                       // maxPacketBits
   80);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxG726::ms_codecInfo32(
   SdpCodec::SDP_CODEC_G726_32,    // codecType
   "G726-32",                 // codecVersion
   false,                      // usesNetEq
   8000,                       // samplingRate
   8,                          // numBitsPerSample
   1,                          // numChannels
   80,                        // interleaveBlockSize
   32000,                      // bitRate. It doesn't matter right now.
   40*8,                       // minPacketBits
   40*8,                       // avgPacketBits
   40*8,                       // maxPacketBits
   80);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxG726::ms_codecInfo40(
   SdpCodec::SDP_CODEC_G726_40,    // codecType
   "G726-40",                 // codecVersion
   false,                      // usesNetEq
   8000,                       // samplingRate
   8,                          // numBitsPerSample
   1,                          // numChannels
   80,                        // interleaveBlockSize
   40000,                      // bitRate. It doesn't matter right now.
   50*8,                       // minPacketBits
   50*8,                       // avgPacketBits
   50*8,                       // maxPacketBits
   80);                       // numSamplesPerFrame

MpeSipxG726::MpeSipxG726(int payloadType, G726_BITRATE bitRate)
: MpEncoderBase(payloadType, getCodecInfo(bitRate))
{
}

MpeSipxG726::~MpeSipxG726()
{
}

OsStatus MpeSipxG726::initEncode(void)
{
   m_pG726state = g726_init(NULL, getInfo()->getBitRate(), G726_ENCODING_LINEAR, G726_PACKING_LEFT);

   if (m_pG726state)
   {
      return OS_SUCCESS;
   }
   else
   {
      return OS_FAILED;
   }
}

OsStatus MpeSipxG726::freeEncode(void)
{
   int res = g726_release(m_pG726state);

   if (res == 0)
   {
      return OS_SUCCESS;
   }
   else
   {
      return OS_FAILED;
   }
}

OsStatus MpeSipxG726::encode(const MpAudioSample* pAudioSamples,
                            const int numSamples,
                            int& rSamplesConsumed,
                            unsigned char* pCodeBuf,
                            const int bytesLeft,
                            int& rSizeInBytes,
                            UtlBoolean& sendNow,
                            MpAudioBuf::SpeechType& rAudioCategory)
{
   assert(numSamples == 80); // we expect 10ms frames

   rSizeInBytes = g726_encode(m_pG726state, pCodeBuf, pAudioSamples, numSamples);
   unsigned maxPacketBits = getInfo()->getMaxPacketBits();
   assert(rSizeInBytes == maxPacketBits/8);

   rSamplesConsumed = numSamples;
   if (rSizeInBytes > 0)
   {
      sendNow = TRUE;
   }
   else
   {
      sendNow = FALSE;
   }
   rAudioCategory = MpAudioBuf::MP_SPEECH_UNKNOWN;

   return OS_SUCCESS;
}

const MpCodecInfo* MpeSipxG726::getCodecInfo(G726_BITRATE bitRate)
{
   const MpCodecInfo* pCodecInfo = &ms_codecInfo40;
   switch(bitRate)
   {
   case MpeSipxG726::BITRATE_16:
      pCodecInfo = &ms_codecInfo16;
      break;
   case MpeSipxG726::BITRATE_24:
      pCodecInfo = &ms_codecInfo24;
      break;
   case MpeSipxG726::BITRATE_32:
      pCodecInfo = &ms_codecInfo32;
      break;
   case MpeSipxG726::BITRATE_40:
      pCodecInfo = &ms_codecInfo40;
      break;
   default:
      ;
   }

   return pCodecInfo;
}

#endif /* HAVE_SPAN_DSP ] */
