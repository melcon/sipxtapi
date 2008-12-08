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
#include <os/OsDateTime.h>
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
, m_pAnswerSipMessage(NULL)
, m_pSecurity(NULL)
{

}

CpSdpNegotiation::~CpSdpNegotiation()
{
   delete m_pSecurity;
   m_pSecurity = NULL;

   delete m_pOfferSipMessage;
   m_pOfferSipMessage = NULL;

   delete m_pAnswerSipMessage;
   m_pAnswerSipMessage = NULL;
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

   delete m_pAnswerSipMessage;
   m_pAnswerSipMessage = NULL;
}

void CpSdpNegotiation::sdpOfferFinished(const SipMessage& rOfferSipMessage)
{
   m_bSdpOfferFinished = TRUE;

   delete m_pOfferSipMessage;
   m_pOfferSipMessage = new SipMessage(rOfferSipMessage); // keep copy of sdp offer
}

void CpSdpNegotiation::sdpAnswerFinished(const SipMessage& rAnswerSipMessage)
{
   if (m_bSdpOfferFinished)
   {
      delete m_pAnswerSipMessage;
      m_pAnswerSipMessage = new SipMessage(rAnswerSipMessage); // keep copy of sdp answer

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

void CpSdpNegotiation::setSecurity(const SIPXTACK_SECURITY_ATTRIBUTES* val)
{
   delete m_pSecurity;
   m_pSecurity = NULL;

   if (val)
   {
      m_pSecurity = new SIPXTACK_SECURITY_ATTRIBUTES(*val);
   }
}

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

CpSdpNegotiation::SdpBodyType CpSdpNegotiation::getSdpBodyType(const SipMessage& sipMessage) const
{
   if (m_pOfferSipMessage)
   {
      UtlBoolean bMatch = compareSipMessages(*m_pOfferSipMessage, sipMessage);
      if (bMatch)
      {
         return CpSdpNegotiation::SDP_BODY_OFFER; // this is an offer
      }
   }
   if (m_pAnswerSipMessage)
   {
      UtlBoolean bMatch = compareSipMessages(*m_pAnswerSipMessage, sipMessage);
      if (bMatch)
      {
         return CpSdpNegotiation::SDP_BODY_ANSWER; // this is an answer
      }
   }

   // get pointer to internal sdp body
   const SdpBody* pSdpBody = sipMessage.getSdpBody(m_pSecurity);
   if (pSdpBody)
   {
      // message has SDP body, but we are unable to tell if its offer or answer
      return CpSdpNegotiation::SDP_BODY_UNKNOWN;
   }

   // there is no sdp body
   return CpSdpNegotiation::SDP_BODY_NONE;
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

UtlBoolean CpSdpNegotiation::compareSipMessages(const SipMessage& baseSipMessage, const SipMessage& receivedSipMessage) const
{
   int receivedSeqNum;
   UtlString receivedSeqMethod;
   UtlBoolean bReceivedIsRequest;
   receivedSipMessage.getCSeqField(&receivedSeqNum, &receivedSeqMethod);
   bReceivedIsRequest = receivedSipMessage.isRequest();

   int baseSeqNum;
   UtlString baseSeqMethod;
   UtlBoolean bBaseIsRequest;
   baseSipMessage.getCSeqField(&baseSeqNum, &baseSeqMethod);
   bBaseIsRequest = baseSipMessage.isRequest();

   const SdpBody* pBaseSdpBody = baseSipMessage.getSdpBody(m_pSecurity);
   const SdpBody* pReceivedSdpBody = receivedSipMessage.getSdpBody(m_pSecurity);

   if (pBaseSdpBody && pReceivedSdpBody)
   {
      // both messages have SDP body
      if (baseSeqNum == receivedSeqNum && baseSeqMethod.compareTo(receivedSeqMethod) == 0)
      {
         if (bReceivedIsRequest && bBaseIsRequest)
         {
            // both are requests
            return TRUE;
         }
         else if (!bReceivedIsRequest && !bBaseIsRequest)
         {
            // both are responses, also response codes must match
            int receivedResponseCode = receivedSipMessage.getResponseStatusCode();
            int baseResponseCode = baseSipMessage.getResponseStatusCode();
            if (receivedResponseCode == baseResponseCode)
            {
               return TRUE;
            }
         }
      }
   }

   return FALSE;
}

/* ============================ FUNCTIONS ================================= */
