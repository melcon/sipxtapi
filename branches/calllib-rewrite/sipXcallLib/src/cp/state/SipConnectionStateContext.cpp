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
#include <net/SipMessage.h>
#include <cp/CpDefs.h>
#include <cp/state/SipConnectionStateContext.h>

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

SipConnectionStateContext::SipConnectionStateContext()
: m_mediaSessionState(SipConnectionStateContext::MEDIA_SESSION_NONE)
, m_previousMediaSessionState(SipConnectionStateContext::MEDIA_SESSION_NONE)
, m_localMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_NONE)
, m_remoteMediaConnectionState(SipConnectionStateContext::MEDIA_CONNECTION_NONE)
, m_allowedRemote(NULL)
, m_implicitAllowedRemote("INVITE, ACK, CANCEL, BYE, OPTIONS, REGISTER")
, m_100relSetting(CP_100REL_PREFER_RELIABLE)
, m_contactId(AUTOMATIC_CONTACT_ID)
, m_rtpTransport(RTP_TRANSPORT_UDP)
, m_pSecurity(NULL)
, m_pLastReceivedInvite(NULL)
, m_pLastSent2xxToInvite(NULL)
, m_bUseLocalHoldSDP(FALSE)
, m_updateSetting(CP_SIP_UPDATE_ONLY_INBOUND)
, m_bRedirecting(FALSE)
, m_bAckReceived(FALSE)
, m_bCancelSent(FALSE)
, m_bByeSent(FALSE)
, m_iByeRetryCount(0)
, m_pByeRetryTimer(NULL)
, m_pCancelTimeoutTimer(NULL)
, m_pByeTimeoutTimer(NULL)
, m_pSessionRenegotiationTimer(NULL)
, m_iRenegotiationRetryCount(0)
, m_p2xxInviteRetransmitTimer(NULL)
, m_i2xxInviteRetransmitCount(0)
, m_pSessionTimeoutCheckTimer(NULL)
, m_pSessionRefreshTimer(NULL)
{
   m_sipClientTransactionMgr.setSipTransactionListener(&m_100RelTracker);
}

SipConnectionStateContext::~SipConnectionStateContext()
{
   delete m_pSecurity;
   m_pSecurity = NULL;
   delete m_pLastReceivedInvite;
   m_pLastReceivedInvite = NULL;
   delete m_pLastSent2xxToInvite;
   m_pLastSent2xxToInvite = NULL;
   delete m_pByeRetryTimer;
   m_pByeRetryTimer = NULL;
   delete m_pCancelTimeoutTimer;
   m_pCancelTimeoutTimer = NULL;
   delete m_pByeTimeoutTimer;
   m_pByeTimeoutTimer = NULL;
   delete m_pSessionRenegotiationTimer;
   m_pSessionRenegotiationTimer = NULL;
   delete m_p2xxInviteRetransmitTimer;
   m_p2xxInviteRetransmitTimer = NULL;
   delete m_pSessionTimeoutCheckTimer;
   m_pSessionTimeoutCheckTimer = NULL;
   delete m_pSessionRefreshTimer;
   m_pSessionRefreshTimer = NULL;
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
