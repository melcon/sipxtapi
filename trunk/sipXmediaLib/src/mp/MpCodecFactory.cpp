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
#include "mp/MpCodecFactory.h"
#include "os/OsSysLog.h"

// all encoder child classes
#include "mp/MpePtAVT.h"
#include "mp/MpeSipxPcma.h"
#include "mp/MpeSipxPcmu.h"

#ifdef HAVE_SPEEX // [
#include "mp/MpeSipxSpeex.h"
#ifdef ENABLE_WIDEBAND_AUDIO
#include "mp/MpeSipxSpeexWb.h"
#include "mp/MpeSipxSpeexUWb.h"
#endif
#endif // HAVE_SPEEX ]

#ifdef HAVE_INTEL_IPP // [
#ifdef ENABLE_WIDEBAND_AUDIO
#include "mp/MpeIPPG7221.h"
#include "mp/MpeIPPG7291.h"
#include "mp/MpeIPPGAmrWb.h"
#endif
#include "mp/MpeIPPGAmr.h"
#include "mp/MpeIPPG728.h"
#include "mp/MpeIPPG729.h"
#include "mp/MpeIPPG729i.h"
#include "mp/MpeIPPG7231.h"
#endif // HAVE_IPP ]

// All decoder child classes
#include "mp/MpdPtAVT.h"
#include "mp/MpdSipxPcma.h"
#include "mp/MpdSipxPcmu.h"

#ifdef HAVE_SPEEX // [
#include "mp/MpdSipxSpeex.h"
#ifdef ENABLE_WIDEBAND_AUDIO
#include "mp/MpdSipxSpeexWb.h"
#include "mp/MpdSipxSpeexUWb.h"
#endif
#endif // HAVE_SPEEX ]

#ifdef ENABLE_WIDEBAND_AUDIO
#include "mp/MpdSipxL16.h"
#include "mp/MpeSipxL16.h"
#endif

#ifdef HAVE_SPAN_DSP // [
#include "mp/MpdSipxG726.h"
#include "mp/MpeSipxG726.h"
#ifdef ENABLE_WIDEBAND_AUDIO
#include "mp/MpdSipxG722.h"
#include "mp/MpeSipxG722.h"
#endif // ENABLE_WIDEBAND_AUDIO ]
#endif // HAVE_SPAN_DSP ]

#ifdef HAVE_GSM // [
#include "mp/MpdSipxGSM.h"
#include "mp/MpeSipxGSM.h"
#endif // HAVE_GSM ]

#ifdef HAVE_ILBC // [
#include "mp/MpdSipxILBC.h"
#include "mp/MpeSipxILBC.h"
#endif // HAVE_ILBC ]

#ifdef HAVE_INTEL_IPP // [
#ifdef ENABLE_WIDEBAND_AUDIO
#include "mp/MpdIPPG7221.h"
#include "mp/MpdIPPG7291.h"
#include "mp/MpdIPPGAmrWb.h"
#endif
#include "mp/MpdIPPGAmr.h"
#include "mp/MpdIPPG728.h"
#include "mp/MpdIPPG729.h"
#include "mp/MpdIPPG729i.h"
#include "mp/MpdIPPG7231.h"
#endif // HAVE_IPP ]

MpCodecFactory MpCodecFactory::sInstance;

/* ============================ CREATORS ================================== */

MpCodecFactory* MpCodecFactory::getMpCodecFactory(void)
{
   return &sInstance;
}

MpCodecFactory::MpCodecFactory(void)
{
}

//:Destructor
MpCodecFactory::~MpCodecFactory()
{
}

/* ============================ MANIPULATORS ============================== */


// Returns a new instance of a decoder of the indicated type
// param: internalCodecId - (in) codec type identifier
// param: payloadType - (in) RTP payload type associated with this decoder
// param: rpDecoder - (out) Reference to a pointer to the new decoder object
OsStatus MpCodecFactory::createDecoder(SdpCodec::SdpCodecTypes internalCodecId,
                        int payloadType, MpDecoderBase*& rpDecoder)
{
   rpDecoder=NULL;

   switch (internalCodecId)
   {
   case (SdpCodec::SDP_CODEC_TONES):
      rpDecoder = new MpdPtAVT(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_PCMA):
      rpDecoder = new MpdSipxPcma(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_PCMU):
      rpDecoder = new MpdSipxPcmu(payloadType);
      break;
#ifdef HAVE_SPEEX // [
   case (SdpCodec::SDP_CODEC_SPEEX_5):
   case (SdpCodec::SDP_CODEC_SPEEX_8):
   case (SdpCodec::SDP_CODEC_SPEEX_11):
   case (SdpCodec::SDP_CODEC_SPEEX_15):
   case (SdpCodec::SDP_CODEC_SPEEX_18):
   case (SdpCodec::SDP_CODEC_SPEEX_24):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;
#ifdef ENABLE_WIDEBAND_AUDIO
   case (SdpCodec::SDP_CODEC_SPEEX_WB_9):
   case (SdpCodec::SDP_CODEC_SPEEX_WB_12):
   case (SdpCodec::SDP_CODEC_SPEEX_WB_16):
   case (SdpCodec::SDP_CODEC_SPEEX_WB_20):
   case (SdpCodec::SDP_CODEC_SPEEX_WB_23):
   case (SdpCodec::SDP_CODEC_SPEEX_WB_27):
   case (SdpCodec::SDP_CODEC_SPEEX_WB_34):
   case (SdpCodec::SDP_CODEC_SPEEX_WB_42):
      rpDecoder = new MpdSipxSpeexWb(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_11):
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_14):
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_18):
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_22):
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_25):
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_29):
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_36):
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_44):
      rpDecoder = new MpdSipxSpeexUWb(payloadType);
      break;
#endif // ENABLE_WIDEBAND_AUDIO ]
#endif // HAVE_SPEEX ]
#ifdef ENABLE_WIDEBAND_AUDIO
      // L16 codecs
   case (SdpCodec::SDP_CODEC_L16_8000_MONO):
      rpDecoder = new MpdSipxL16(payloadType, 8000);
      break;
   case (SdpCodec::SDP_CODEC_L16_11025_MONO):
      rpDecoder = new MpdSipxL16(payloadType, 11025);
      break;
   case (SdpCodec::SDP_CODEC_L16_16000_MONO):
      rpDecoder = new MpdSipxL16(payloadType, 16000);
      break;
   case (SdpCodec::SDP_CODEC_L16_22050_MONO):
      rpDecoder = new MpdSipxL16(payloadType, 22050);
      break;
   case (SdpCodec::SDP_CODEC_L16_24000_MONO):
      rpDecoder = new MpdSipxL16(payloadType, 24000);
      break;
   case (SdpCodec::SDP_CODEC_L16_32000_MONO):
      rpDecoder = new MpdSipxL16(payloadType, 32000);
      break;
   case (SdpCodec::SDP_CODEC_L16_44100_MONO):
      rpDecoder = new MpdSipxL16(payloadType, 44100);
      break;
   case (SdpCodec::SDP_CODEC_L16_48000_MONO):
      rpDecoder = new MpdSipxL16(payloadType, 48000);
      break;
#endif
#ifdef HAVE_SPAN_DSP // [
   case (SdpCodec::SDP_CODEC_G726_16):
      rpDecoder = new MpdSipxG726(payloadType, MpdSipxG726::BITRATE_16);
      break;
   case (SdpCodec::SDP_CODEC_G726_24):
      rpDecoder = new MpdSipxG726(payloadType, MpdSipxG726::BITRATE_24);
      break;
   case (SdpCodec::SDP_CODEC_G726_32):
      rpDecoder = new MpdSipxG726(payloadType, MpdSipxG726::BITRATE_32);
      break;
   case (SdpCodec::SDP_CODEC_G726_40):
      rpDecoder = new MpdSipxG726(payloadType, MpdSipxG726::BITRATE_40);
      break;
#ifdef ENABLE_WIDEBAND_AUDIO
   case (SdpCodec::SDP_CODEC_G722):
      rpDecoder = new MpdSipxG722(payloadType);
      break;
#endif // ENABLE_WIDEBAND_AUDIO ]
#endif // HAVE_SPAN_DSP ]
#ifdef HAVE_GSM // [
   case (SdpCodec::SDP_CODEC_GSM):
      rpDecoder = new MpdSipxGSM(payloadType);
      break;
#endif // HAVE_GSM ]
#ifdef HAVE_ILBC // [
   case (SdpCodec::SDP_CODEC_ILBC):
      rpDecoder = new MpdSipxILBC(payloadType, 30);
      break;
   case (SdpCodec::SDP_CODEC_ILBC_20MS):
      rpDecoder = new MpdSipxILBC(payloadType, 20);
      break;
#endif // HAVE_ILBC ]
#ifdef HAVE_INTEL_IPP // [
#ifdef ENABLE_WIDEBAND_AUDIO
   case (SdpCodec::SDP_CODEC_G7221_16): 
      rpDecoder = new MpdIPPG7221(payloadType, 16000);
      break;
   case (SdpCodec::SDP_CODEC_G7221_24): 
      rpDecoder = new MpdIPPG7221(payloadType, 24000);
      break;
   case (SdpCodec::SDP_CODEC_G7221_32): 
      rpDecoder = new MpdIPPG7221(payloadType, 32000);
      break;
   case (SdpCodec::SDP_CODEC_AMR_WB_12650): 
      rpDecoder = new MpdIPPGAmrWb(payloadType, 12650, FALSE);
      break;
   case (SdpCodec::SDP_CODEC_AMR_WB_23850): 
      rpDecoder = new MpdIPPGAmrWb(payloadType, 23850, TRUE);
      break;
   case (SdpCodec::SDP_CODEC_G7291_8000): 
   case (SdpCodec::SDP_CODEC_G7291_12000): 
   case (SdpCodec::SDP_CODEC_G7291_14000): 
   case (SdpCodec::SDP_CODEC_G7291_16000): 
   case (SdpCodec::SDP_CODEC_G7291_18000): 
   case (SdpCodec::SDP_CODEC_G7291_20000): 
   case (SdpCodec::SDP_CODEC_G7291_22000): 
   case (SdpCodec::SDP_CODEC_G7291_24000): 
   case (SdpCodec::SDP_CODEC_G7291_26000): 
   case (SdpCodec::SDP_CODEC_G7291_28000): 
   case (SdpCodec::SDP_CODEC_G7291_30000): 
   case (SdpCodec::SDP_CODEC_G7291_32000): 
      rpDecoder = new MpdIPPG7291(payloadType);
      break;
#endif
   case (SdpCodec::SDP_CODEC_AMR_4750): 
      rpDecoder = new MpdIPPGAmr(payloadType, 4750, FALSE);
      break;
   case (SdpCodec::SDP_CODEC_AMR_10200): 
      rpDecoder = new MpdIPPGAmr(payloadType, 10200, TRUE);
      break;
   case (SdpCodec::SDP_CODEC_G723): 
      rpDecoder = new MpdIPPG7231(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_G728):
      rpDecoder = new MpdIPPG728(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_G729):
      rpDecoder = new MpdIPPG729(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_G729D):
      rpDecoder = new MpdIPPG729i(payloadType, 6400); // also supports 8000
      break;
   case (SdpCodec::SDP_CODEC_G729E):
      rpDecoder = new MpdIPPG729i(payloadType, 11800);
      break;
#endif // HAVE_IPP ]
   default:
      OsSysLog::add(FAC_MP, PRI_WARNING, 
                    "MpCodecFactory::createDecoder unknown codec type "
                    "internalCodecId = (SdpCodec::SdpCodecTypes) %d, "
                    "payloadType = %d",
                    internalCodecId, payloadType);
      assert(FALSE);
      break;
   }

   if (rpDecoder)
   {
      return OS_SUCCESS;
   }

   return OS_INVALID_ARGUMENT;
}

// Returns a new instance of an encoder of the indicated type
// param: internalCodecId - (in) codec type identifier
// param: payloadType - (in) RTP payload type associated with this encoder
// param: rpEncoder - (out) Reference to a pointer to the new encoder object

OsStatus MpCodecFactory::createEncoder(SdpCodec::SdpCodecTypes internalCodecId,
                          int payloadType, MpEncoderBase*& rpEncoder)
{
   rpEncoder=NULL;
   switch (internalCodecId) {

   case (SdpCodec::SDP_CODEC_TONES):
      rpEncoder = new MpePtAVT(payloadType);
      break;

   case (SdpCodec::SDP_CODEC_PCMA):
      rpEncoder = new MpeSipxPcma(payloadType);
      break;

   case (SdpCodec::SDP_CODEC_PCMU):
      rpEncoder = new MpeSipxPcmu(payloadType);
      break;

#ifdef HAVE_SPEEX // [
   case (SdpCodec::SDP_CODEC_SPEEX_5):
      rpEncoder = new MpeSipxSpeex(payloadType, 2);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_8):
      rpEncoder = new MpeSipxSpeex(payloadType, 3);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_11):
      rpEncoder = new MpeSipxSpeex(payloadType, 4);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_15):
      rpEncoder = new MpeSipxSpeex(payloadType, 5);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_18):
      rpEncoder = new MpeSipxSpeex(payloadType, 6);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_24):
      rpEncoder = new MpeSipxSpeex(payloadType, 7);
      break;
#ifdef ENABLE_WIDEBAND_AUDIO
   case (SdpCodec::SDP_CODEC_SPEEX_WB_9):
      rpEncoder = new MpeSipxSpeexWb(payloadType, 3);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_WB_12):
      rpEncoder = new MpeSipxSpeexWb(payloadType, 4);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_WB_16):
      rpEncoder = new MpeSipxSpeexWb(payloadType, 5);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_WB_20):
      rpEncoder = new MpeSipxSpeexWb(payloadType, 6);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_WB_23):
      rpEncoder = new MpeSipxSpeexWb(payloadType, 7);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_WB_27):
      rpEncoder = new MpeSipxSpeexWb(payloadType, 8);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_WB_34):
      rpEncoder = new MpeSipxSpeexWb(payloadType, 9);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_WB_42):
      rpEncoder = new MpeSipxSpeexWb(payloadType, 10);
      break;
   // speex ultra wide band
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_11):
      rpEncoder = new MpeSipxSpeexUWb(payloadType, 3);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_14):
      rpEncoder = new MpeSipxSpeexUWb(payloadType, 4);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_18):
      rpEncoder = new MpeSipxSpeexUWb(payloadType, 5);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_22):
      rpEncoder = new MpeSipxSpeexUWb(payloadType, 6);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_25):
      rpEncoder = new MpeSipxSpeexUWb(payloadType, 7);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_29):
      rpEncoder = new MpeSipxSpeexUWb(payloadType, 8);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_36):
      rpEncoder = new MpeSipxSpeexUWb(payloadType, 9);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_UWB_44):
      rpEncoder = new MpeSipxSpeexUWb(payloadType, 10);
      break;
#endif // ENABLE_WIDEBAND_AUDIO ]
#endif // HAVE_SPEEX ]
#ifdef ENABLE_WIDEBAND_AUDIO
      // L16 codecs
   case (SdpCodec::SDP_CODEC_L16_8000_MONO):
      rpEncoder = new MpeSipxL16(payloadType, 8000);
      break;
   case (SdpCodec::SDP_CODEC_L16_11025_MONO):
      rpEncoder = new MpeSipxL16(payloadType, 11025);
      break;
   case (SdpCodec::SDP_CODEC_L16_16000_MONO):
      rpEncoder = new MpeSipxL16(payloadType, 16000);
      break;
   case (SdpCodec::SDP_CODEC_L16_22050_MONO):
      rpEncoder = new MpeSipxL16(payloadType, 22050);
      break;
   case (SdpCodec::SDP_CODEC_L16_24000_MONO):
      rpEncoder = new MpeSipxL16(payloadType, 24000);
      break;
   case (SdpCodec::SDP_CODEC_L16_32000_MONO):
      rpEncoder = new MpeSipxL16(payloadType, 32000);
      break;
   case (SdpCodec::SDP_CODEC_L16_44100_MONO):
      rpEncoder = new MpeSipxL16(payloadType, 44100);
      break;
   case (SdpCodec::SDP_CODEC_L16_48000_MONO):
      rpEncoder = new MpeSipxL16(payloadType, 48000);
      break;
#endif
#ifdef HAVE_SPAN_DSP // [
   case (SdpCodec::SDP_CODEC_G726_16):
      rpEncoder = new MpeSipxG726(payloadType, MpeSipxG726::BITRATE_16);
      break;
   case (SdpCodec::SDP_CODEC_G726_24):
      rpEncoder = new MpeSipxG726(payloadType, MpeSipxG726::BITRATE_24);
      break;
   case (SdpCodec::SDP_CODEC_G726_32):
      rpEncoder = new MpeSipxG726(payloadType, MpeSipxG726::BITRATE_32);
      break;
   case (SdpCodec::SDP_CODEC_G726_40):
      rpEncoder = new MpeSipxG726(payloadType, MpeSipxG726::BITRATE_40);
      break;
#ifdef ENABLE_WIDEBAND_AUDIO
   case (SdpCodec::SDP_CODEC_G722):
      rpEncoder = new MpeSipxG722(payloadType);
      break;
#endif // ENABLE_WIDEBAND_AUDIO ]
#endif // HAVE_SPAN_DSP ]
#ifdef HAVE_GSM // [
   case (SdpCodec::SDP_CODEC_GSM):
      rpEncoder = new MpeSipxGSM(payloadType);
      break;
#endif // HAVE_GSM ]
#ifdef HAVE_ILBC // [
   case (SdpCodec::SDP_CODEC_ILBC):
      rpEncoder = new MpeSipxILBC(payloadType, 30);
      break;
   case (SdpCodec::SDP_CODEC_ILBC_20MS):
      rpEncoder = new MpeSipxILBC(payloadType, 20);
      break;
#endif // HAVE_ILBC ]
#ifdef HAVE_INTEL_IPP // [
#ifdef ENABLE_WIDEBAND_AUDIO
   case (SdpCodec::SDP_CODEC_G7221_16): 
      rpEncoder = new MpeIPPG7221(payloadType, 16000);
      break;
   case (SdpCodec::SDP_CODEC_G7221_24): 
      rpEncoder = new MpeIPPG7221(payloadType, 24000);
      break;
   case (SdpCodec::SDP_CODEC_G7221_32): 
      rpEncoder = new MpeIPPG7221(payloadType, 32000);
      break;
   case (SdpCodec::SDP_CODEC_AMR_WB_12650): 
      rpEncoder = new MpeIPPGAmrWb(payloadType, 12650, FALSE);
      break;
   case (SdpCodec::SDP_CODEC_AMR_WB_23850): 
      rpEncoder = new MpeIPPGAmrWb(payloadType, 23850, TRUE);
      break;
   case (SdpCodec::SDP_CODEC_G7291_8000):
      rpEncoder = new MpeIPPG7291(payloadType, 8000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_12000):
      rpEncoder = new MpeIPPG7291(payloadType, 12000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_14000):
      rpEncoder = new MpeIPPG7291(payloadType, 14000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_16000):
      rpEncoder = new MpeIPPG7291(payloadType, 16000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_18000):
      rpEncoder = new MpeIPPG7291(payloadType, 18000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_20000):
      rpEncoder = new MpeIPPG7291(payloadType, 20000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_22000):
      rpEncoder = new MpeIPPG7291(payloadType, 22000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_24000):
      rpEncoder = new MpeIPPG7291(payloadType, 24000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_26000):
      rpEncoder = new MpeIPPG7291(payloadType, 26000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_28000):
      rpEncoder = new MpeIPPG7291(payloadType, 28000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_30000):
      rpEncoder = new MpeIPPG7291(payloadType, 30000);
      break;
   case (SdpCodec::SDP_CODEC_G7291_32000):
      rpEncoder = new MpeIPPG7291(payloadType, 32000);
      break;
#endif
   case (SdpCodec::SDP_CODEC_AMR_4750): 
      rpEncoder = new MpeIPPGAmr(payloadType, 4750, FALSE);
      break;
   case (SdpCodec::SDP_CODEC_AMR_10200): 
      rpEncoder = new MpeIPPGAmr(payloadType, 10200, TRUE);
      break;
   case (SdpCodec::SDP_CODEC_G723):
      rpEncoder = new MpeIPPG7231(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_G728):
      rpEncoder = new MpeIPPG728(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_G729):
      rpEncoder = new MpeIPPG729(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_G729D):
      rpEncoder = new MpeIPPG729i(payloadType, 6400);
      break;
   case (SdpCodec::SDP_CODEC_G729E):
      rpEncoder = new MpeIPPG729i(payloadType, 11800);
      break;
#endif // HAVE_IPP ]

   default:
      OsSysLog::add(FAC_MP, PRI_WARNING, 
                    "MpCodecFactory::createEncoder unknown codec type "
                    "internalCodecId = (SdpCodec::SdpCodecTypes) %d, "
                    "payloadType = %d",
                    internalCodecId, payloadType);
      assert(FALSE);
      break;
   }

   if (rpEncoder)
   {
      return OS_SUCCESS;
   }

   return OS_INVALID_ARGUMENT;
}
