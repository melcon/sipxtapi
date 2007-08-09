//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2005-2007 SIPez LLC.
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

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/SipXSecurityEventListener.h"
#include "tapi/SipXEvents.h"

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

SipXSecurityEventListener::SipXSecurityEventListener( SIPX_INST pInst )
   : m_pInst(pInst)
{

}

SipXSecurityEventListener::~SipXSecurityEventListener()
{

}

/* ============================ MANIPULATORS ============================== */

void SipXSecurityEventListener::OnEncrypt( const SipSecurityEvent& event )
{
   sipxFireSecurityEvent(m_pInst,
                         event.m_sSRTPkey,
                         event.m_pCertificate,
                         event.m_nCertificateSize,
                         SECURITY_ENCRYPT,
                         event.m_Cause,
                         event.m_sSubjAltName,
                         event.m_SessionCallId,
                         event.m_sRemoteAddress);
}

void SipXSecurityEventListener::OnDecrypt( const SipSecurityEvent& event )
{
   sipxFireSecurityEvent(m_pInst,
                         event.m_sSRTPkey,
                         event.m_pCertificate,
                         event.m_nCertificateSize,
                         SECURITY_DECRYPT,
                         event.m_Cause,
                         event.m_sSubjAltName,
                         event.m_SessionCallId,
                         event.m_sRemoteAddress);
}

void SipXSecurityEventListener::OnTLS( const SipSecurityEvent& event )
{
   sipxFireSecurityEvent(m_pInst,
                         event.m_sSRTPkey,
                         event.m_pCertificate,
                         event.m_nCertificateSize,
                         SECURITY_TLS,
                         event.m_Cause,
                         event.m_sSubjAltName,
                         event.m_SessionCallId,
                         event.m_sRemoteAddress);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

