//  
// Copyright (C) 2006 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//  
// Copyright (C) 2006 SIPfoundry Inc. 
// Licensed by SIPfoundry under the LGPL license. 
//  
// $$ 
////////////////////////////////////////////////////////////////////////////// 

#include "assert.h"
// APPLICATION INCLUDES
#include "mp/MpeSipxL16.h"
#include "mp/NetInTask.h"

MpeSipxL16::MpeSipxL16(int payloadType, int samplesPerSec)
: MpEncoderBase(payloadType, getCodecInfo(samplesPerSec))
{
}

MpeSipxL16::~MpeSipxL16()
{
}

OsStatus MpeSipxL16::initEncode(void)
{
   // L16 has no state
   return OS_SUCCESS;
}

OsStatus MpeSipxL16::freeEncode(void)
{
   return OS_SUCCESS;
}

OsStatus MpeSipxL16::encode(const MpAudioSample* pAudioSamples,
                            const int numSamples,
                            int& rSamplesConsumed,
                            unsigned char* pCodeBuf,
                            const int bytesLeft,
                            int& rSizeInBytes,
                            UtlBoolean& sendNow,
                            MpSpeechType& rAudioCategory)
{
   int freeBufferCapacity = bytesLeft / sizeof(MpAudioSample); // free buffer in samples
   int i = 0;
   for (i = 0; i < numSamples && i < freeBufferCapacity; i++)
   {
      ((MpAudioSample*)pCodeBuf)[i] = htons(pAudioSamples[i]);
   }
   rSizeInBytes = i * sizeof(MpAudioSample);
   rSamplesConsumed = i;

   if (rSizeInBytes > 0)
   {
      sendNow = TRUE;
   }
   else
   {
      sendNow = FALSE;
   }
   rAudioCategory = MP_SPEECH_UNKNOWN;

   return OS_SUCCESS;
}

const MpCodecInfo* MpeSipxL16::getCodecInfo(int samplesPerSec)
{
   const MpCodecInfo* pCodecInfo = &ms_codecInfo8000;
   switch(samplesPerSec)
   {
   case 8000:
      pCodecInfo = &ms_codecInfo8000;
      break;
   case 11025:
      pCodecInfo = &ms_codecInfo11025;
      break;
   case 16000:
      pCodecInfo = &ms_codecInfo16000;
      break;
   case 22050:
      pCodecInfo = &ms_codecInfo22050;
      break;
   case 24000:
      pCodecInfo = &ms_codecInfo24000;
      break;
   case 32000:
      pCodecInfo = &ms_codecInfo32000;
      break;
   case 44100:
      pCodecInfo = &ms_codecInfo44100;
      break;
   case 48000:
      pCodecInfo = &ms_codecInfo48000;
      break;
   default:
      ;
   }

   return pCodecInfo;
}

const MpCodecInfo MpeSipxL16::ms_codecInfo8000(
   SdpCodec::SDP_CODEC_L16_8000_MONO,    // codecType
   "",                         // codecVersion
   8000,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   8000*2*8,                   // bitRate. It doesn't matter right now.
   80*2*8,                       // minPacketBits
   80*2*8,                       // maxPacketBits
   80);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxL16::ms_codecInfo11025(
   SdpCodec::SDP_CODEC_L16_11025_MONO,    // codecType
   "",                         // codecVersion
   11025,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   11025*2*8,                   // bitRate. It doesn't matter right now.
   110*2*8,                       // minPacketBits
   111*2*8,                       // maxPacketBits
   110);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxL16::ms_codecInfo16000(
   SdpCodec::SDP_CODEC_L16_16000_MONO,    // codecType
   "",                         // codecVersion
   16000,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   16000*2*8,                   // bitRate. It doesn't matter right now.
   160*2*8,                       // minPacketBits
   160*2*8,                       // maxPacketBits
   160);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxL16::ms_codecInfo22050(
   SdpCodec::SDP_CODEC_L16_22050_MONO,    // codecType
   "",                         // codecVersion
   22050,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   22050*2*8,                   // bitRate. It doesn't matter right now.
   220*2*8,                       // minPacketBits
   221*2*8,                       // maxPacketBits
   220);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxL16::ms_codecInfo24000(
   SdpCodec::SDP_CODEC_L16_24000_MONO,    // codecType
   "",                         // codecVersion
   24000,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   24000*2*8,                   // bitRate. It doesn't matter right now.
   240*2*8,                       // minPacketBits
   240*2*8,                       // maxPacketBits
   240);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxL16::ms_codecInfo32000(
   SdpCodec::SDP_CODEC_L16_32000_MONO,    // codecType
   "",                         // codecVersion
   32000,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   32000*2*8,                   // bitRate. It doesn't matter right now.
   320*2*8,                       // minPacketBits
   320*2*8,                       // maxPacketBits
   320);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxL16::ms_codecInfo44100(
   SdpCodec::SDP_CODEC_L16_44100_MONO,    // codecType
   "",                         // codecVersion
   44100,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   44100*2*8,                   // bitRate. It doesn't matter right now.
   441*2*8,                       // minPacketBits
   441*2*8,                       // maxPacketBits
   441);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxL16::ms_codecInfo48000(
   SdpCodec::SDP_CODEC_L16_48000_MONO,    // codecType
   "",                         // codecVersion
   48000,                       // samplingRate
   16,                          // numBitsPerSample
   1,                          // numChannels
   48000*2*8,                   // bitRate. It doesn't matter right now.
   480*2*8,                       // minPacketBits
   480*2*8,                       // maxPacketBits
   480);                       // numSamplesPerFrame
