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
        "Telephone event",
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
   case SdpCodec::SDP_CODEC_G728:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G728,
         SdpCodec::SDP_CODEC_G728,
         "G728",
         "G.728 16 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G728,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_G729:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G729,
         SdpCodec::SDP_CODEC_G729,
         "G729B",
         "G.729 annex B",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G729,
         8000,
         20000,
         1,
         "annexb=yes",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_G729D:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G729D,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G729D",
         "G.729 annex D",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G729D,
         8000,
         20000,
         1,
         "annexb=yes",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_G729E:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G729E,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G729E",
         "G.729 annex E",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G729E,
         8000,
         20000,
         1,
         "annexb=yes",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_G723:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G723,
         SdpCodec::SDP_CODEC_G723,
         "G723.1",
         "G.723.1 5.3 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G723,
         8000,
         30000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW); 
      break;
   case SdpCodec::SDP_CODEC_G7221_16:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G7221_16,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G722.1_16",
         "G.722.1 16 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G7221,
         16000,
         20000,
         1,
         "bitrate=16000",
         SdpCodec::SDP_CODEC_CPU_NORMAL,
         SDP_CODEC_BANDWIDTH_LOW); 
      break;
   case SdpCodec::SDP_CODEC_G7221_24:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G7221_24,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G722.1_24",
         "G.722.1 24 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G7221,
         16000,
         20000,
         1,
         "bitrate=24000",
         SdpCodec::SDP_CODEC_CPU_NORMAL,
         SDP_CODEC_BANDWIDTH_NORMAL); 
      break;
   case SdpCodec::SDP_CODEC_G7221_32:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G7221_32,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G722.1_32",
         "G.722.1 32 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G7221,
         16000,
         20000,
         1,
         "bitrate=32000",
         SdpCodec::SDP_CODEC_CPU_NORMAL,
         SDP_CODEC_BANDWIDTH_NORMAL); 
      break;
#endif // HAVE_INTEL_IPP
   case SdpCodec::SDP_CODEC_PCMA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_PCMA,
         SdpCodec::SDP_CODEC_PCMA,
         "PCMA",
         "G.711 a-law",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_PCMA,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
   case SdpCodec::SDP_CODEC_PCMU:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_PCMU,
         SdpCodec::SDP_CODEC_PCMU,
         "PCMU",
         "G.711 u-law",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_PCMU,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
   case SdpCodec::SDP_CODEC_ILBC:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_ILBC,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "ILBC",
         "iLBC 30ms",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_ILBC,
         8000,
         30000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_NORMAL,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_ILBC_20MS:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_ILBC_20MS,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "ILBC-20MS",
         "iLBC 20ms",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_ILBC,
         8000,
         20000,
         1,
         "mode=20",
         SdpCodec::SDP_CODEC_CPU_NORMAL,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
#ifdef HAVE_GSM
   case SdpCodec::SDP_CODEC_GSM:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_GSM,
         SdpCodec::SDP_CODEC_GSM,
         "GSM",
         "GSM full rate",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_GSM,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_NORMAL,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
#endif // HAVE_GSM
#ifdef HAVE_SPEEX
   case SdpCodec::SDP_CODEC_SPEEX_5:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_5,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_5",
         "Speex 5.95 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=2",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_8:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_8,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_8",
         "Speex 8 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=3",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_11:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_11,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_11",
         "Speex 11 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=4",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_15:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_15,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_15",
         "Speex 15 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=5",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_18:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_18,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_18",
         "Speex 18.2 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=6",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_24:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_24,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_24",
         "Speex 24.6 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=7",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
#ifdef ENABLE_WIDEBAND_AUDIO
   case SdpCodec::SDP_CODEC_SPEEX_WB_9:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_WB_9,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_WB_9",
         "Speex wb 9.8 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         16000,
         20000,
         1,
         "mode=3",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_WB_12:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_WB_12,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_WB_12",
         "Speex wb 12.8 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         16000,
         20000,
         1,
         "mode=4",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_WB_16:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_WB_16,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_WB_16",
         "Speex wb 16.8 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         16000,
         20000,
         1,
         "mode=5",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_WB_20:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_WB_20,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_WB_20",
         "Speex wb 20.6 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         16000,
         20000,
         1,
         "mode=6",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_WB_23:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_WB_23,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_WB_23",
         "Speex wb 23.8 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         16000,
         20000,
         1,
         "mode=7",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_WB_27:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_WB_27,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_WB_27",
         "Speex wb 27.8 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         16000,
         20000,
         1,
         "mode=8",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_WB_34:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_WB_34,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_WB_34",
         "Speex wb 34.4 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         16000,
         20000,
         1,
         "mode=9",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_WB_42:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_WB_42,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_WB_42",
         "Speex wb 42.4 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         16000,
         20000,
         1,
         "mode=10",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
      // speex ultra wide band codecs
   case SdpCodec::SDP_CODEC_SPEEX_UWB_11:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_UWB_11,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_UWB_11",
         "Speex uwb 11.6 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         32000,
         20000,
         1,
         "mode=3",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_UWB_14:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_UWB_14,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_UWB_14",
         "Speex uwb 14.6 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         32000,
         20000,
         1,
         "mode=4",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_UWB_18:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_UWB_18,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_UWB_18",
         "Speex uwb 18.6 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         32000,
         20000,
         1,
         "mode=5",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_UWB_22:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_UWB_22,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_UWB_22",
         "Speex uwb 22.4 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         32000,
         20000,
         1,
         "mode=6",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_UWB_25:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_UWB_25,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_UWB_25",
         "Speex uwb 25.6 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         32000,
         20000,
         1,
         "mode=7",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_UWB_29:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_UWB_29,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_UWB_29",
         "Speex uwb 29.6 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         32000,
         20000,
         1,
         "mode=8",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_UWB_36:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_UWB_36,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_UWB_36",
         "Speex uwb 36.0 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         32000,
         20000,
         1,
         "mode=9",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_UWB_44:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_UWB_44,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_UWB_44",
         "Speex uwb 44.0 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         32000,
         20000,
         1,
         "mode=10",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;

#endif // ENABLE_WIDEBAND_AUDIO ]
#endif // HAVE_SPEEX
#ifdef HAVE_SPAN_DSP
   case SdpCodec::SDP_CODEC_G722:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G722,
         SdpCodec::SDP_CODEC_G722,
         "G722",
         "G.722 64 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G722,
         16000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
   case SdpCodec::SDP_CODEC_G726_16:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G726_16,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G726_16",
         "G.726 16 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G726_16,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_G726_24:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G726_24,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G726_24",
         "G.726 24 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G726_24,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_G726_32:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G726_32,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G726_32",
         "G.726 32 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G726_32,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_G726_40:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G726_40,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G726_40",
         "G.726 40 kbit/s",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G726_40,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
#endif // HAVE_SPAN_DSP
   // L16 codec (uncompressed 16bit audio)
   case SdpCodec::SDP_CODEC_L16_8000_MONO:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_L16_8000_MONO,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "L16_8000_MONO",
         "L16 8Khz mono",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_L16,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_L16_11025_MONO:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_L16_11025_MONO,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "L16_11025_MONO",
         "L16 11Khz mono",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_L16,
         11025,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_L16_16000_MONO:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_L16_16000_MONO,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "L16_16000_MONO",
         "L16 16Khz mono",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_L16,
         16000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_L16_22050_MONO:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_L16_22050_MONO,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "L16_22050_MONO",
         "L16 22Khz mono",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_L16,
         22050,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
   case SdpCodec::SDP_CODEC_L16_24000_MONO:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_L16_24000_MONO,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "L16_24000_MONO",
         "L16 24Khz mono",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_L16,
         24000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
   case SdpCodec::SDP_CODEC_L16_32000_MONO:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_L16_32000_MONO,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "L16_32000_MONO",
         "L16 32Khz mono",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_L16,
         32000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
   case SdpCodec::SDP_CODEC_L16_44100_MONO:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_L16_44100_MONO,
         SdpCodec::SDP_CODEC_L16_44100_MONO,
         "L16_44100_MONO",
         "L16 44Khz mono",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_L16,
         44100,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
   case SdpCodec::SDP_CODEC_L16_48000_MONO:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_L16_48000_MONO,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "L16_48000_MONO",
         "L16 48Khz mono",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_L16,
         48000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH);
      break;
#ifdef VIDEO
   case SdpCodec::SDP_CODEC_VP71_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-CIF",
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
      UtlString result;
      if (audioCodecs.length() > 0)
      {
         result.appendFormat("%s ", MIME_SUBTYPE_DTMF_TONES);
         result.append(audioCodecs);
      }
      else
      {
         result = MIME_SUBTYPE_DTMF_TONES;
      }
      return result;
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

