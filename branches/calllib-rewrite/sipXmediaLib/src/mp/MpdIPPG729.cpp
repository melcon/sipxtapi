//
// Copyright (C) 2005 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_INTEL_IPP /* [ */

#ifdef WIN32 // [
#   pragma comment(lib, "ipps.lib")
#   pragma comment(lib, "ippsc.lib")
#   pragma comment(lib, "ippcore.lib")
#   pragma comment(lib, "ippsr.lib")
#   pragma comment(lib, "libircmt.lib")
#endif // WIN32 ]

// APPLICATION INCLUDES
#include "mp/MpdIPPG729.h"

extern "C" {
#include "ippcore.h"
#include "ipps.h"
#include "usccodec.h"
}

#define G729_PATTERN_LENGTH 20

const MpCodecInfo MpdIPPG729::smCodecInfo(
   SdpCodec::SDP_CODEC_G729,    // codecType
   "Intel IPP 6.0",             // codecVersion
   8000,                        // samplingRate
   16,                          // numBitsPerSample (not used)
   1,                           // numChannels
   8000,                        // bitRate
   20*8,                        // minPacketBits
   20*8,                        // maxPacketBits
   160);                        // numSamplesPerFrame - 20ms frame

MpdIPPG729::MpdIPPG729(int payloadType)
: MpDecoderBase(payloadType, &smCodecInfo)
{
   codec = (LoadedCodec*)malloc(sizeof(LoadedCodec));
   memset(codec, 0, sizeof(LoadedCodec));
}

MpdIPPG729::~MpdIPPG729()
{
   freeDecode();
   free(codec);
}

OsStatus MpdIPPG729::initDecode()
{
   int lCallResult;

   ippStaticInit();

   switch (getPayloadType())
   {
   case SdpCodec::SDP_CODEC_G729: 
      // Apply codec name and VAD to codec definition structure
      strcpy((char*)codec->codecName, "IPP_G729A");
      codec->lIsVad = 0;
      break;
   default:
      return OS_FAILED;
   }

   // Load codec by name
   lCallResult = LoadUSCCodecByName(codec, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Get USC codec params
   lCallResult = USCCodecAllocInfo(&codec->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   lCallResult = USCCodecGetInfo(&codec->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Get its supported format details
   lCallResult = GetUSCCodecParamsByFormat(codec, BY_NAME, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Set params for decode
   USC_PCMType streamType;
   streamType.bitPerSample = getInfo()->getNumBitsPerSample();
   streamType.nChannels = getInfo()->getNumChannels();
   streamType.sample_frequency = getInfo()->getSamplingRate();

   // decoder doesn't need to know PCM type
   lCallResult = SetUSCDecoderPCMType(&codec->uscParams, -1, &streamType, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // instead of SetUSCDecoderParams(...)
   codec->uscParams.pInfo->params.direction = USC_DECODE;
   codec->uscParams.pInfo->params.law = 0;

   // Prepare input buffer parameters
   Bitstream.bitrate = 8000;
   Bitstream.frametype = 3;
   Bitstream.nbytes = 10;

   // Alloc memory for the codec
   lCallResult = USCCodecAlloc(&codec->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Init decoder
   lCallResult = USCDecoderInit(&codec->uscParams, NULL, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED; 
   }

   return OS_SUCCESS;
}

OsStatus MpdIPPG729::freeDecode(void)
{
   // Free codec memory
   USCFree(&codec->uscParams);

   return OS_SUCCESS;
}

int MpdIPPG729::decode(const MpRtpBufPtr &rtpPacket,
                       unsigned decodedBufferLength,
                       MpAudioSample *samplesBuffer) 
{
   if (!rtpPacket.isValid())
      return 0;

   unsigned payloadSize = rtpPacket->getPayloadSize();
   unsigned maxPayloadSize = smCodecInfo.getMaxPacketBits()/8;

   assert(payloadSize <= maxPayloadSize);
   if (payloadSize > maxPayloadSize)
   {
      return 0;
   }

   // Each decode pattern must have 10 bytes or less (in case VAD enabled)
   int frames = payloadSize / Bitstream.nbytes;

   // Setup input and output pointers
   Bitstream.pBuffer = const_cast<char*>(rtpPacket->getDataPtr());
   PCMStream.pBuffer = reinterpret_cast<char*>(samplesBuffer);
   // zero the buffer in case we decode less than 320 bytes
   // as it happens sometimes
   memset(PCMStream.pBuffer, 0, 320);
   
   // Decode frames
   for (int i = 0; i < frames; i++)
   {
      // Decode one frame
      USCCodecDecode(&codec->uscParams, &Bitstream, &PCMStream, 0);

      // move pointers
      Bitstream.pBuffer += Bitstream.nbytes;
      PCMStream.pBuffer += codec->uscParams.pInfo->params.framesize;
   }

   // Return number of decoded samples
   return 160;
}

#endif /* !HAVE_INTEL_IPP ] */
