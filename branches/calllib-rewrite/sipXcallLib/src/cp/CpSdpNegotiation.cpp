//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <sdp/SdpCodecList.h>
#include <net/SdpBody.h>
#include <cp/CpSdpNegotiation.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

CpSdpNegotiation::CpSdpNegotiation()
: m_negotiationState(CpSdpNegotiation::SDP_NOT_YET_NEGOTIATED)
, m_bSdpOfferFinished(FALSE)
, m_bSdpAnswerFinished(FALSE)
, m_bLocallyInitiated(FALSE)
, m_sdpOfferingMode(CpSdpNegotiation::SDP_OFFERING_IMMEDIATE)
{

}

CpSdpNegotiation::~CpSdpNegotiation()
{

}

/* ============================ MANIPULATORS ============================== */

void CpSdpNegotiation::startSdpNegotiation(UtlBoolean bLocallyInitiated /*= TRUE*/)
{
   m_negotiationState = CpSdpNegotiation::SDP_NEGOTIATION_IN_PROGRESS;
   m_bSdpOfferFinished = FALSE;
   m_bSdpAnswerFinished = FALSE;
   m_bLocallyInitiated = bLocallyInitiated;
}

void CpSdpNegotiation::sdpOfferFinished()
{
   m_bSdpOfferFinished = TRUE;
}

void CpSdpNegotiation::sdpAnswerFinished()
{
   if (m_bSdpOfferFinished)
   {
      m_bSdpAnswerFinished = TRUE;
      m_negotiationState = CpSdpNegotiation::SDP_NEGOTIATION_COMPLETE;
   }
}

void CpSdpNegotiation::getCommonSdpCodecs(const SdpBody& rSdpBody, ///< SDP body
                                          const SdpCodecList& supportedCodecs,
                                          int& numCodecsInCommon, ///< how many codecs do we have in common
                                          SdpCodecList& commonCodecsForEncoder,
                                          SdpCodecList& commonCodecsForDecoder,
                                          UtlString& remoteRtpAddress,
                                          int& remoteRtpPort,
                                          int& remoteRtcpPort,
                                          int& remoteVideoRtpPort,
                                          int& remoteVideoRtcpPort,
                                          const SdpSrtpParameters& localSrtpParams,
                                          SdpSrtpParameters& matchingSrtpParams,
                                          int localBandwidth,
                                          int& matchingBandwidth,
                                          int localVideoFramerate,
                                          int& matchingVideoFramerate)
{
   memset((void*)&matchingSrtpParams, 0, sizeof(SdpSrtpParameters));

   SdpCodec** encoderCodecs = NULL;
   SdpCodec** decoderCodecs = NULL;

   rSdpBody.getBestAudioCodecs(supportedCodecs,
      numCodecsInCommon,
      encoderCodecs,
      decoderCodecs,
      remoteRtpAddress,
      remoteRtpPort,
      remoteRtcpPort,
      remoteVideoRtpPort,
      remoteVideoRtcpPort,
      localSrtpParams,
      matchingSrtpParams,
      localBandwidth,
      matchingBandwidth,
      localVideoFramerate,
      matchingVideoFramerate);

   // To be compliant with RFC 3264
   if(rSdpBody.findValueInField("a", "sendonly"))
   {
      remoteRtpAddress = "0.0.0.0";
   }

   // convert array to SdpCodecList
   commonCodecsForEncoder.clearCodecs();
   commonCodecsForEncoder.addCodecs(numCodecsInCommon, encoderCodecs);
   commonCodecsForDecoder.clearCodecs();
   commonCodecsForDecoder.addCodecs(numCodecsInCommon, decoderCodecs);

   // Free up the codec copies and array
   for(int codecIndex = 0; codecIndex < numCodecsInCommon; codecIndex++)
   {
      delete encoderCodecs[codecIndex];
      encoderCodecs[codecIndex] = NULL;

      delete decoderCodecs[codecIndex];
      decoderCodecs[codecIndex] = NULL;
   }
   if(encoderCodecs) delete[] encoderCodecs;
   encoderCodecs = NULL;

   if(decoderCodecs) delete[] decoderCodecs;
   decoderCodecs = NULL;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

UtlBoolean CpSdpNegotiation::isNewSdpNegotiationAllowed() const
{
   return m_negotiationState != CpSdpNegotiation::SDP_NEGOTIATION_IN_PROGRESS;
}

UtlBoolean CpSdpNegotiation::isSdpNegotiationInProgress() const
{
   return m_negotiationState == CpSdpNegotiation::SDP_NEGOTIATION_IN_PROGRESS;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
