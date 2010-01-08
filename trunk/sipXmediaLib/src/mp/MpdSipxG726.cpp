//  
// Copyright (C) 2006-2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//  
// Copyright (C) 2006-2007 SIPfoundry Inc. 
// Licensed by SIPfoundry under the LGPL license. 
//  
// $$ 
////////////////////////////////////////////////////////////////////////////// 

#ifdef HAVE_SPAN_DSP /* [ */

// APPLICATION INCLUDES
#include "mp/MpdSipxG726.h"

const MpCodecInfo MpdSipxG726::ms_codecInfo16(
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

const MpCodecInfo MpdSipxG726::ms_codecInfo24(
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

const MpCodecInfo MpdSipxG726::ms_codecInfo32(
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

const MpCodecInfo MpdSipxG726::ms_codecInfo40(
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

MpdSipxG726::MpdSipxG726(int payloadType, G726_BITRATE bitRate)
: MpDecoderBase(payloadType, getCodecInfo(bitRate))
{
}

MpdSipxG726::~MpdSipxG726()
{
}

OsStatus MpdSipxG726::initDecode()
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

OsStatus MpdSipxG726::freeDecode(void)
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

int MpdSipxG726::decode(const MpRtpBufPtr &pPacket, unsigned decodedBufferLength, MpAudioSample *samplesBuffer)
{
   // Assert that available buffer size is enough for the packet.
   assert(pPacket->getPayloadSize() == getInfo()->getMaxPacketBits()/8);
   if (pPacket->getPayloadSize() != getInfo()->getMaxPacketBits()/8)
      return 0;

   if (decodedBufferLength < 80)
   {
      osPrintf("MpdSipxG726::decode: Jitter buffer overloaded. Glitch!\n");
      return 0;
   }

   int samplesDecoded = g726_decode(m_pG726state, samplesBuffer, (const uint8_t*)pPacket->getDataPtr(), (int)pPacket->getPayloadSize());
   assert(samplesDecoded == 80);

   if (samplesDecoded == 80)
   {
      return samplesDecoded;
   }
   else
   {
      return 0;
   }
}

const MpCodecInfo* MpdSipxG726::getCodecInfo(G726_BITRATE bitRate)
{
   const MpCodecInfo* pCodecInfo = &ms_codecInfo40;
   switch(bitRate)
   {
   case MpdSipxG726::BITRATE_16:
      pCodecInfo = &ms_codecInfo16;
      break;
   case MpdSipxG726::BITRATE_24:
      pCodecInfo = &ms_codecInfo24;
      break;
   case MpdSipxG726::BITRATE_32:
      pCodecInfo = &ms_codecInfo32;
      break;
   case MpdSipxG726::BITRATE_40:
      pCodecInfo = &ms_codecInfo40;
      break;
   default:
      ;
   }

   return pCodecInfo;
}

#endif /* HAVE_SPAN_DSP ] */
