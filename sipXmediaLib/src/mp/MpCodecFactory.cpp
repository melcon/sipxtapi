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

   case (SdpCodec::SDP_CODEC_SPEEX):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;

   case (SdpCodec::SDP_CODEC_SPEEX_5):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;

    case (SdpCodec::SDP_CODEC_SPEEX_15):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;

    case (SdpCodec::SDP_CODEC_SPEEX_24):
      rpDecoder = new MpdSipxSpeex(payloadType);
      break;

#endif // HAVE_SPEEX ]

#ifdef HAVE_GSM // [
   case (SdpCodec::SDP_CODEC_GSM):
      rpDecoder = new MpdSipxGSM(payloadType);
      break;
#endif // HAVE_GSM ]

#ifdef HAVE_ILBC // [
   case (SdpCodec::SDP_CODEC_ILBC):
      rpDecoder = new MpdSipxILBC(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_ILBC_20MS):
      rpDecoder = new MpdSipxILBC(payloadType);
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
   case (SdpCodec::SDP_CODEC_SPEEX):
      rpEncoder = new MpeSipxSpeex(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_SPEEX_5):
      rpEncoder = new MpeSipxSpeex(payloadType, 2);
      break;
    case (SdpCodec::SDP_CODEC_SPEEX_15):
      rpEncoder = new MpeSipxSpeex(payloadType, 5);
      break;
    case (SdpCodec::SDP_CODEC_SPEEX_24):
      rpEncoder = new MpeSipxSpeex(payloadType, 7);
      break;
#endif // HAVE_SPEEX ]

#ifdef HAVE_GSM // [
   case (SdpCodec::SDP_CODEC_GSM):
      rpEncoder = new MpeSipxGSM(payloadType);
      break;
#endif // HAVE_GSM ]

#ifdef HAVE_ILBC // [
   case (SdpCodec::SDP_CODEC_ILBC):
      rpEncoder = new MpeSipxILBC(payloadType);
      break;
   case (SdpCodec::SDP_CODEC_ILBC_20MS):
      rpEncoder = new MpeSipxILBC(payloadType);
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
