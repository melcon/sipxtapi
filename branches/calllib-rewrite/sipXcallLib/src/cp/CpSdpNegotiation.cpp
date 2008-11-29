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
#include <sdp/SdpCodec.h>
#include <sdp/SdpCodecList.h>
#include <net/SdpBody.h>
#include <net/SipMessage.h>
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
, m_pOfferSipMessage(NULL)
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

   delete m_pOfferSipMessage;
   m_pOfferSipMessage = NULL;
}

void CpSdpNegotiation::sdpOfferFinished(const SipMessage& rOfferSipMessage)
{
   m_bSdpOfferFinished = TRUE;

   delete m_pOfferSipMessage;
   m_pOfferSipMessage = new SipMessage(rOfferSipMessage); // keep copy of sdp offer
}

void CpSdpNegotiation::sdpAnswerFinished()
{
   if (m_bSdpOfferFinished)
   {
      m_bSdpAnswerFinished = TRUE;
      m_negotiationState = CpSdpNegotiation::SDP_NEGOTIATION_COMPLETE;
   }
}

void CpSdpNegotiation::getCommonSdpCodecs(const SdpBody& rSdpBody, ///< inbound SDP body (offer or answer)
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

   deleteSdpCodecArray(numCodecsInCommon, encoderCodecs);
   deleteSdpCodecArray(numCodecsInCommon, decoderCodecs);
}

void CpSdpNegotiation::addSdpBody(SipMessage& rSipMessage,///< sip message where we want to add SDP body
                                  int nRtpContacts,
                                  UtlString hostAddresses[],
                                  int rtpAudioPorts[],
                                  int rtcpAudiopPorts[],
                                  int rtpVideoPorts[],
                                  int rtcpVideoPorts[],
                                  RTP_TRANSPORT transportTypes[],
                                  const SdpCodecList& sdpCodecList,
                                  const SdpSrtpParameters& srtpParams,
                                  int videoBandwidth,
                                  int videoFramerate,
                                  RTP_TRANSPORT rtpTransportOptions)
{
   int codecCount = sdpCodecList.getCodecCount();
   SdpCodec** pCodecArray = new SdpCodec*[codecCount];
   sdpCodecList.getCodecs(codecCount, pCodecArray); // fill array with codec copies

   // if m_pOfferSipMessage is known, then SDP answer is added, otherwise SDP offer
   rSipMessage.addSdpBody(nRtpContacts,
      hostAddresses,
      rtpAudioPorts,
      rtcpAudiopPorts,
      rtpVideoPorts,
      rtcpVideoPorts,
      transportTypes,
      codecCount,
      pCodecArray,
      srtpParams,
      videoBandwidth,
      videoFramerate,
      m_pOfferSipMessage, // original SIP message with offer if available
      rtpTransportOptions);

   deleteSdpCodecArray(codecCount, pCodecArray);
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

UtlBoolean CpSdpNegotiation::isSdpNegotiationComplete() const
{
   return m_negotiationState == CpSdpNegotiation::SDP_NEGOTIATION_COMPLETE;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void CpSdpNegotiation::deleteSdpCodecArray(int arraySize, SdpCodec**& sdpCodecArray)
{
   // Free up the codec copies and array
   for(int codecIndex = 0; codecIndex < arraySize; codecIndex++)
   {
      delete sdpCodecArray[codecIndex];
      sdpCodecArray[codecIndex] = NULL;
   }
   if(sdpCodecArray) delete[] sdpCodecArray;
   sdpCodecArray = NULL;
}

/* ============================ FUNCTIONS ================================= */
