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
#include "mp/MpdSipxG722.h"

const MpCodecInfo MpdSipxG722::ms_codecInfo64(
   SdpCodec::SDP_CODEC_G722,    // codecType
   "",                 // codecVersion
   16000,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   64000,                      // bitRate. It doesn't matter right now.
   80*8,                       // minPacketBits
   80*8,                       // maxPacketBits
   160);                       // numSamplesPerFrame - 10ms frame

MpdSipxG722::MpdSipxG722(int payloadType)
: MpDecoderBase(payloadType, getCodecInfo())
{
}

MpdSipxG722::~MpdSipxG722()
{
}

OsStatus MpdSipxG722::initDecode()
{
   m_pG722state = g722_decode_init(NULL, getInfo()->getBitRate(), 0);

   if (m_pG722state)
   {
      return OS_SUCCESS;
   }
   else
   {
      return OS_FAILED;
   }
}

OsStatus MpdSipxG722::freeDecode(void)
{
   int res = g722_decode_release(m_pG722state);

   if (res == 0)
   {
      return OS_SUCCESS;
   }
   else
   {
      return OS_FAILED;
   }
}

int MpdSipxG722::decode(const MpRtpBufPtr &pPacket, unsigned decodedBufferLength, MpAudioSample *samplesBuffer)
{
   // do not accept frames longer than 20ms from RTP to protect against buffer overflow
   assert(pPacket->getPayloadSize() <= 160*8);
   if (pPacket->getPayloadSize() > 160*8)
      return 0;

   if (decodedBufferLength < 320) // must be enough for decoding 20ms frame
   {
      osPrintf("MpdSipxG722::decode: Jitter buffer overloaded. Glitch!\n");
      return 0;
   }

   int samplesDecoded = g722_decode(m_pG722state, samplesBuffer, (const uint8_t*)pPacket->getDataPtr(), (int)pPacket->getPayloadSize());

   return samplesDecoded;
}

const MpCodecInfo* MpdSipxG722::getCodecInfo()
{
   return &ms_codecInfo64;
}

#endif /* HAVE_SPAN_DSP ] */
