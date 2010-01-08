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

#ifndef SipConnectionStateContext_h__
#define SipConnectionStateContext_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlSList.h>
#include <net/SipTagGenerator.h>
#include <net/SmimeBody.h>
#include <cp/XSipConnectionContext.h>
#include <cp/Cp100RelTracker.h>
#include <cp/CpSdpNegotiation.h>
#include <cp/CpSipTransactionManager.h>
#include <cp/CpSessionTimerProperties.h>
#include <tapi/sipXtapi.h> // craziness

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class OsTimer;

/**
 * SipConnectionStateContext contains public members which are visible only to state itself
 * and don't need to be locked when accessed.
 *
 * When accessing members of XSipConnectionContext, this class needs to be locked as those
 * members are public.
 */
class SipConnectionStateContext : public XSipConnectionContext
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /**
    * MediaSessionState tracks state of media session - if RTP is flowing or not.
    */
   typedef enum
   {
      MEDIA_SESSION_NONE = 0, ///< initial state
      MEDIA_SESSION_FULLY_HELD, ///< media session is held
      MEDIA_SESSION_LOCALLY_HELD, ///< media session is held
      MEDIA_SESSION_REMOTELY_HELD, ///< media session is held
      MEDIA_SESSION_ACTIVE, ///< media session is active
   } MediaSessionState;

   /**
   * MediaConnectionState tracks state of media connection. Local represents audio sent from us
   * remote represents audio sent from remote party.
   */
   typedef enum
   {
      MEDIA_CONNECTION_NONE = 0, ///< initial state
      MEDIA_CONNECTION_ACTIVE, ///< initial state
      MEDIA_CONNECTION_HELD, ///< initial state
   } MediaConnectionState;

   MediaSessionState m_mediaSessionState; ///< keeps track of media session state (active, held etc)
   MediaSessionState m_previousMediaSessionState; ///< keeps track of previous media session state (active, held etc)
   MediaConnectionState m_localMediaConnectionState; ///< keeps track of local media connection state
   MediaConnectionState m_remoteMediaConnectionState; ///< keeps track of remote media connection state
   CpSdpNegotiation m_sdpNegotiation; ///< tracks state of SDP negotiation
   UtlString m_allowedRemote;  ///< methods supported by the other side
   UtlString m_implicitAllowedRemote; ///< methods which are allowed implicitly
   SipTagGenerator m_sipTagGenerator; ///< generator for sip tags
   Cp100RelTracker m_100RelTracker; ///< tracker for 100rel responses and PRACKs
   CpSipTransactionManager m_sipClientTransactionMgr; ///< sip outbound transaction tracking
   CpSipTransactionManager m_sipServerTransactionMgr; ///< sip inbound transaction tracking
   UtlString m_locationHeader; ///< value of sip location header
   int m_contactId; ///< id of contact we use. Can be used to lookup SIPX_CONTACT_ADDRESS
   SIPXTACK_SECURITY_ATTRIBUTES* m_pSecurity; ///< security configuration for S/MIME
   RTP_TRANSPORT m_rtpTransport;
   CpSessionTimerProperties m_sessionTimerProperties; ///< properties of session timer (RFC4028)
   SipMessage* m_pLastSentInvite; ///< last sent INVITE
   SipMessage* m_pLastReceivedInvite; ///< last received INVITE
   SipMessage* m_pLastSent2xxToInvite; ///< last sent 2xx response to INVITE (sent until ACK is received)
   UtlBoolean m_bUseLocalHoldSDP; ///< whether we use local hold SDP when offering or answering
   UtlBoolean m_bSdpRenegotiationUseUpdate; ///< use UPDATE method for media session renegotiation
   UtlSList m_redirectContactList; ///< contact URIs which should be followed in redirect
   UtlBoolean m_bRedirecting; ///< TRUE if we are following redirection

   // members used during call tear down
   UtlBoolean m_bAckReceived; ///< TRUE if ACK was received for our sent 200 OK. Needed to make decision for callee if we may send BYE.
   UtlBoolean m_bCallDisconnecting; ///< call is being disconnected. Either CANCEL, BYE or 403 Forbidden was sent
   UtlBoolean m_bByeSent; ///< TRUE when we are disconnecting and BYE was already sent
   int m_iByeRetryCount; ///< counter when retrying BYE for inbound call
   // timers
   OsTimer* m_pByeRetryTimer; ///< timer started if drop is attempted for inbound call, but call cannot be dropped at current state
   OsTimer* m_pCancelTimeoutTimer; ///< timer started after CANCEL is sent to force drop connection if timeout
   OsTimer* m_pByeTimeoutTimer; ///< timer started after BYE is sent to force drop connection if timeout
   OsTimer* m_pSessionRenegotiationTimer; ///< timer started when hold/unhold is requested but re-INVITE is in progress
   int m_iRenegotiationRetryCount; ///< how many times we retried m_pSessionRenegotiationTimer
   OsTimer* m_p2xxInviteRetransmitTimer; ///< timer started when 2xx response to invite is sent, shut down when ack is received
   int m_i2xxInviteRetransmitCount; ///< how many times we retransmitted 2xx

   /* ============================ CREATORS ================================== */

   /** Constructor */
   SipConnectionStateContext();

   /** Destructor */
   ~SipConnectionStateContext();

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   SipConnectionStateContext(const SipConnectionStateContext& rhs);

   SipConnectionStateContext& operator=(const SipConnectionStateContext& rhs);

};

#endif // SipConnectionStateContext_h__
