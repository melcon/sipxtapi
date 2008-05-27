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

#ifdef HAVE_INTEL_IPP // [

#include "assert.h"
// APPLICATION INCLUDES
#include "winsock2.h"
#include "mp/MpeIPPG729.h"

extern "C" {
#include "ippcore.h"
#include "ipps.h"
#include "usccodec.h"
}

const MpCodecInfo MpeIPPG729::smCodecInfo(
   SdpCodec::SDP_CODEC_G729A,    // codecType
   "Intel IPP 5.3",              // codecVersion
   true,                         // usesNetEq
   8000,                         // samplingRate
   16,                           // numBitsPerSample
   1,                            // numChannels
   160,                           // interleaveBlockSize
   8000,                         // bitRate
   20*8,                           // minPacketBits
   20*8,                           // avgPacketBits
   20*8,                          // maxPacketBits
   160);                          // numSamplesPerFrame

MpeIPPG729::MpeIPPG729(int payloadType)
: MpEncoderBase(payloadType, &smCodecInfo)
, inputBuffer(NULL)
, outputBuffer(NULL)
{
   codec = (LoadedCodec*)malloc(sizeof(LoadedCodec));
   memset(codec, 0, sizeof(LoadedCodec));
}

MpeIPPG729::~MpeIPPG729()
{
   freeEncode();
   free(codec);
}

OsStatus MpeIPPG729::initEncode(void)
{
   int lCallResult;

   ippStaticInit();
   strcpy((char*)codec->codecName, "IPP_G729A");
   codec->lIsVad = 1;

   // Load codec by name from command line
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

   // Set params for encode
   USC_PCMType streamType;
   streamType.bitPerSample = getInfo()->getNumBitsPerSample();
   streamType.nChannels = getInfo()->getNumChannels();
   streamType.sample_frequency = getInfo()->getSamplingRate();

   lCallResult = SetUSCEncoderPCMType(&codec->uscParams, LINEAR_PCM, &streamType, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // instead of SetUSCEncoderParams(...)
   codec->uscParams.pInfo->params.direction = USC_ENCODE;
   codec->uscParams.pInfo->params.law = 0;

   // Alloc memory for the codec
   lCallResult = USCCodecAlloc(&codec->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Init decoder
   lCallResult = USCEncoderInit(&codec->uscParams, NULL, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;  
   }

   // Allocate memory for the output buffer. Size of output buffer is equal
   // to the size of 1 frame
   inputBuffer = 
      (Ipp8s *)ippsMalloc_8s(codec->uscParams.pInfo->params.framesize);
   outputBuffer = 
      (Ipp8u *)ippsMalloc_8u(codec->uscParams.pInfo->maxbitsize + 1);

   return OS_SUCCESS;
}

OsStatus MpeIPPG729::freeEncode(void)
{
   // Free codec memory
   USCFree(&codec->uscParams);
   if (inputBuffer)
   {
      ippsFree(inputBuffer);
   }
   if (outputBuffer)
   {
      ippsFree(outputBuffer);
   }

   return OS_SUCCESS;
}


OsStatus MpeIPPG729::encode(const short* pAudioSamples,
                            const int numSamples,
                            int& rSamplesConsumed,
                            unsigned char* pCodeBuf,
                            const int bytesLeft,
                            int& rSizeInBytes,
                            UtlBoolean& sendNow,
                            MpAudioBuf::SpeechType& rAudioCategory)
{

   ippsSet_8u(0, (Ipp8u *)outputBuffer, codec->uscParams.pInfo->maxbitsize + 1);

   ippsCopy_8u((unsigned char *)pAudioSamples, (unsigned char *)inputBuffer,
               codec->uscParams.pInfo->params.framesize);     

   assert((codec->uscParams.pInfo->params.framesize / 
          (codec->uscParams.pInfo->params.pcmType.bitPerSample / 8)) == numSamples);

   int frmlen, infrmLen, FrmDataLen;
   USC_PCMStream PCMStream;
   USC_Bitstream Bitstream;

   // Do the pre-procession of the frame
   infrmLen = USCEncoderPreProcessFrame(&codec->uscParams , inputBuffer,
              (Ipp8s *)outputBuffer, &PCMStream, &Bitstream);
   // Encode one frame
   FrmDataLen = USCCodecEncode(&codec->uscParams, &PCMStream, &Bitstream, 0);
   if(FrmDataLen < 0)
   {
      return OS_FAILED;
   }

   infrmLen += FrmDataLen;
   // Do the post-procession of the frame
   frmlen = USCEncoderPostProcessFrame(&codec->uscParams, inputBuffer,
            (Ipp8s *)outputBuffer, &PCMStream, &Bitstream);


   ippsSet_8u(0, pCodeBuf, 10); 

   if (Bitstream.nbytes == 10 || Bitstream.nbytes == 2 )
   {
      for(int k = 0; k < Bitstream.nbytes; ++k)
      {
         pCodeBuf[k] = outputBuffer[6 + k];
      }
   }
   else
   {
      frmlen =0;
   }

   if (Bitstream.nbytes == 2 || Bitstream.nbytes == 0)
   {
      sendNow = TRUE;
   }
   else
   {
      sendNow = FALSE;
   }

   rAudioCategory = MpAudioBuf::MP_SPEECH_UNKNOWN;
   rSamplesConsumed = FrmDataLen / 
                      (codec->uscParams.pInfo->params.pcmType.bitPerSample / 8);

   if (Bitstream.nbytes <= 10 )
   {
      rSizeInBytes = Bitstream.nbytes;
   }
   else
   {
      rSizeInBytes = 0;
   }

   return OS_SUCCESS;
}

#endif // HAVE_INTEL_IPP ]
