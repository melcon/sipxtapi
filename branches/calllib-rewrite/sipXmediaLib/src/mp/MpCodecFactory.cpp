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
#include "mp/MpeIPPG729.h"
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

#ifdef HAVE_SPAN_DSP // [
#include "mp/MpdSipxG726.h"
#include "mp/MpeSipxG726.h"
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
#include "mp/MpdIPPG729.h"
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
   case (SdpCodec::SDP_CODEC_G729):
      rpDecoder = new MpdIPPG729(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_G723): 
      rpDecoder = new MpdIPPG7231(payloadType);
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
   case (SdpCodec::SDP_CODEC_G729):
      rpEncoder = new MpeIPPG729(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_G723):
      rpEncoder = new MpeIPPG7231(payloadType);
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
