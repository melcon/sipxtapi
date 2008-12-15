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
, m_allowedRemote(NULL)
, m_implicitAllowedRemote("INVITE, ACK, CANCEL, BYE, OPTIONS, REGISTER")
, m_contactId(AUTOMATIC_CONTACT_ID)
, m_rtpTransport(RTP_TRANSPORT_UDP)
, m_pSecurity(NULL)
, m_pLastSentInvite(NULL)
, m_pLastReceivedInvite(NULL)
, m_bAckReceived(FALSE)
, m_bCallDisconnecting(FALSE)
, m_iByeRetryCount(0)
, m_pByeRetryTimer(NULL)
, m_pCancelTimer(NULL)
, m_pByeTimer(NULL)
, m_pHoldTimer(NULL)
, m_iHoldRetryCount(0)
{
   m_sipOutTransactionMgr.setSipTransactionListener(&m_100RelTracker);
}

SipConnectionStateContext::~SipConnectionStateContext()
{
   delete m_pSecurity;
   m_pSecurity = NULL;
   delete m_pLastSentInvite;
   m_pLastSentInvite = NULL;
   delete m_pLastReceivedInvite;
   m_pLastReceivedInvite = NULL;
   delete m_pByeRetryTimer;
   m_pByeRetryTimer = NULL;
   delete m_pCancelTimer;
   m_pCancelTimer = NULL;
   delete m_pByeTimer;
   m_pByeTimer = NULL;
   delete m_pHoldTimer;
   m_pHoldTimer = NULL;
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
