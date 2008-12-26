//  
// Copyright (C) 2006 SIPez LLC. 
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

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <sdp/SdpCodecFactory.h>
#include <sdp/SdpCodec.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

SdpCodec* SdpCodecFactory::buildSdpCodec(SdpCodec::SdpCodecTypes codecType)
{
   SdpCodec* pCodec = NULL;
   switch(codecType)
   {
   case SdpCodec::SDP_CODEC_TONES:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_TONES,
        SdpCodec::SDP_CODEC_UNKNOWN,
        "TELEPHONE-EVENT",
        MIME_TYPE_AUDIO,
        MIME_SUBTYPE_DTMF_TONES,
        8000,
        20000,
        1,
        "",
        SdpCodec::SDP_CODEC_CPU_LOW,
        SDP_CODEC_BANDWIDTH_LOW);
      break;
#ifdef HAVE_INTEL_IPP
   case SdpCodec::SDP_CODEC_G729:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G729,
         SdpCodec::SDP_CODEC_G729,
         "G729A",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G729,
         8000,
         20000,
         1,
         "annexb=no",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_G723:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G723,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G723.1",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G723,
         8000,
         30000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW); 
      break;
#endif // HAVE_INTEL_IPP
   case SdpCodec::SDP_CODEC_GIPS_PCMA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_GIPS_PCMA,
         SdpCodec::SDP_CODEC_PCMA,
         "PCMA",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_PCMA,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_GIPS_PCMU:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_GIPS_PCMU,
         SdpCodec::SDP_CODEC_PCMU,
         "PCMU",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_PCMU,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_ILBC:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_ILBC,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "ILBC",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_ILBC,
         8000,
         30000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
#ifdef HAVE_GSM
   case SdpCodec::SDP_CODEC_GSM:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_GSM,
         SdpCodec::SDP_CODEC_GSM,
         "GSM",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_GSM,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
#endif // HAVE_GSM
#ifdef HAVE_SPEEX
   case SdpCodec::SDP_CODEC_SPEEX:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=3",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_5:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_5,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_5",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=2",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_15:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_15,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_15",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=5",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_24:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_24,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_24",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=7",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
#endif // HAVE_SPEEX
#ifdef VIDEO
   case SdpCodec::SDP_CODEC_VP71_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_VP71,
         90000,
         20000,
         1,
         "size=CIF/QCIF/SQCIF",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_VP71_QCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_QCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_VP71,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_VP71_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_SQCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_VP71,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_VP71_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_QVGA,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_VP71,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_QVGA);
      break;
   case SdpCodec::SDP_CODEC_IYUV_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_IYUV_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "IYUV-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_IYUV,
         90000,
         20000,
         1,
         "size=CIF/QCIF/SQCIF",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_IYUV_QCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_IYUV_QCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "IYUV-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_IYUV,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_IYUV_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_IYUV_SQCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "IYUV-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_IYUV,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_IYUV_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_IYUV_QVGA,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "IYUV-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_IYUV,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QVGA);
      break;
   case SdpCodec::SDP_CODEC_I420_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_I420_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "I420-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_I420,
         90000,
         20000,
         1,
         "size=CIF/QCIF/SQCIF",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_I420_QCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_I420_QCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "I420-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_I420,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_I420_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_I420_SQCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "I420-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_I420,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_I420_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_I420_QVGA,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "I420-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_I420,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QVGA);
      break;

   case SdpCodec::SDP_CODEC_RGB24_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_RGB24_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "RGB24-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_RGB24,
         90000,
         20000,
         1,
         "size=CIF/QCIF/SQCIF",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_RGB24_QCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_RGB24_QCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "RGB24-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_RGB24,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_RGB24_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_RGB24_SQCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "RGB24-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_RGB24,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_RGB24_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_RGB24_QVGA,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "RGB24-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_RGB24,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QVGA);
      break;
   case SdpCodec::SDP_CODEC_H263_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_H263_CIF,
         SdpCodec::SDP_CODEC_H263,
         "H263-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_H263,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_H263_QCIF:
      SdpCodec aCodec(SdpCodec::SDP_CODEC_H263_QCIF,
         SdpCodec::SDP_CODEC_H263,
         "H263-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_H263,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_H263_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_H263_SQCIF,
         SdpCodec::SDP_CODEC_H263,
         "H263-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_H263,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_H263_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_H263_QVGA,
         SdpCodec::SDP_CODEC_H263,
         "H263-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_H263,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QVGA);
      break;
#endif // VIDEO
   default:
      ;
   }

   return pCodec;
}

/* ============================ ACCESSORS ================================= */

UtlString SdpCodecFactory::getFixedAudioCodecs(const UtlString& audioCodecs)
{
   UtlString lcAudioCodecs(audioCodecs);
   lcAudioCodecs.toLower();

   if (!lcAudioCodecs.contains(MIME_SUBTYPE_DTMF_TONES))
   {
      // audio/telephone-event is missing, add it
      UtlString res(audioCodecs);
      if (audioCodecs.length() > 0)
      {
         res += " "MIME_SUBTYPE_DTMF_TONES;
      }
      else
      {
         res = MIME_SUBTYPE_DTMF_TONES;
      }
      return res;
   }
   else
   {
      return audioCodecs;
   }
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

