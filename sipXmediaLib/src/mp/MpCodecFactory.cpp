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
   case (SdpCodec::SDP_CODEC_SPEEX_6):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_8):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_11):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_15):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_18):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_24):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;
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
   case (SdpCodec::SDP_CODEC_SPEEX_6):
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
