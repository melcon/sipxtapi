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

#ifndef CpSdpNegotiation_h__
#define CpSdpNegotiation_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/IStunSocket.h>
#include <utl/UtlBool.h>
#include <utl/UtlString.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class SdpCodecList;
class SdpBody;
class SdpCodec;
class SipMessage;
struct SdpSrtpParameters;

/**
 * CpSdpNegotiation is a helper class for tracking SDP negotiation state
 * and various SDP handling functions.
 *
 * It can:
 * - add SDP body (offer or answer) to SipMessage
 * - get common codecs for inbound SDP (offer or answer)
 *
 * SDP negotiation tracking is manual, startSdpNegotiation, sdpOfferFinished
 * and sdpAnswerFinished need to be called.
 */
class CpSdpNegotiation
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   typedef enum
   {
      SDP_NOT_YET_NEGOTIATED, ///< SDP has not been negotiated yet
      SDP_NEGOTIATION_IN_PROGRESS, ///< SDP offer or answer was sent
      SDP_NEGOTIATION_COMPLETE ///< SDP negotiation has been completed
   } SdpNegotiationState;

   /**
   * We support 2 SDP offering modes - immediate and delayed. Immediate sends
   * offer as soon as possible, to be able to receive early audio.
   * Delayed offering sends SDP offer as late as possible. This saves media
   * resources, in case lots of calls are made which might be rejected.
   */
   typedef enum
   {
      SDP_OFFERING_IMMEDIATE = 0, ///< offer SDP in the first request or the first reliable non failure response (rel 1xx or 200) if was not in inbound request
      SDP_OFFERING_DELAYED = 1 ///< do not offer SDP in INVITE, offer it only in the first reliable non failure message
   } SdpOfferingMode;

   /* ============================ CREATORS ================================== */

   /** Constructor */
   CpSdpNegotiation();

   /** Destructor. */
   ~CpSdpNegotiation();

   /* ============================ MANIPULATORS ============================== */

   /** Call before SDP offer is sent or received to initiate SDP negotiation tracking */
   void startSdpNegotiation(UtlBoolean bLocallyInitiated = TRUE);

   /** Call to notify class that SDP offer part of handshake is finished */
   void sdpOfferFinished(const SipMessage& rOfferSipMessage);

   /** Call to notify class that SDP answer part of handshake is finished */
   void sdpAnswerFinished();

   /**
    * Gets sdp codecs for encoder & decoder which are in common with our supported codecs.
    * We separate encoder & decoder codecs, as they may use different dynamic payload type.
    *
    * Caller must make sure that matchingSrtpParams is set on media interface at some point.
    * commonCodecsForEncoder can be used for startRtpSend
    * commonCodecsForDecoder can be used for startRtpReceive
    */
   static void getCommonSdpCodecs(const SdpBody& rSdpBody, ///< inbound SDP body (offer or answer)
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
                                  int& matchingVideoFramerate);

   /**
    * Adds SDP body to specified SIP message. Offer/Answer SDP is added automatically depending
    * on negotiation state.
    */
   void addSdpBody(SipMessage& rSipMessage,///< sip message where we want to add SDP body
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
                   RTP_TRANSPORT rtpTransportOptions = RTP_TRANSPORT_UDP);

   /* ============================ ACCESSORS ================================= */

   /** Gets state of SDP negotiation */
   CpSdpNegotiation::SdpNegotiationState getNegotiationState() const { return m_negotiationState; }

   /** Returns TRUE if SDP offer was sent or received */
   UtlBoolean getSdpOfferFinished() const { return m_bSdpOfferFinished; }

   /** Returns TRUE if SDP answer was sent or received */
   UtlBoolean getSdpAnswerFinished() const { return m_bSdpAnswerFinished; }

   CpSdpNegotiation::SdpOfferingMode getSdpOfferingMode() const { return m_sdpOfferingMode; }
   void setSdpOfferingMode(CpSdpNegotiation::SdpOfferingMode val) { m_sdpOfferingMode = val; }

   /* ============================ INQUIRY =================================== */

   /** Returns TRUE if we may start new SDP negotiation */
   UtlBoolean isNewSdpNegotiationAllowed() const;

   /** Returns TRUE if SDP negotiation is in progress */
   UtlBoolean isSdpNegotiationInProgress() const;

   /** Returns TRUE if SDP negotiation is complete */
   UtlBoolean isSdpNegotiationComplete() const;

   /** Returns TRUE if SDP negotiation was initiated locally (offer was sent) */
   UtlBoolean isLocallyInitiated() const { return m_bLocallyInitiated; }

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   // compatibility with old SdpBody and SipMessage, craziness
   static void deleteSdpCodecArray(int arraySize, SdpCodec**& sdpCodecArray);

   SdpNegotiationState m_negotiationState; ///< keeps track of SDP negotiation state
   UtlBoolean m_bSdpOfferFinished; ///< TRUE if SDP offer was sent or received
   UtlBoolean m_bSdpAnswerFinished; ///< TRUE if SDP answer was sent or received
   UtlBoolean m_bLocallyInitiated; ///< TRUE if we are the SDP negotiation initiator

   SdpOfferingMode m_sdpOfferingMode; ///< configures SDP negotiation mode - immediate or delayed
   SipMessage* m_pOfferSipMessage; ///< Sip message with SDP offer if its available
};

#endif // CpSdpNegotiation_h__
