//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
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

#ifndef SipSecurityEventListener_h__
#define SipSecurityEventListener_h__


// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include "tapi/sipXtapiEvents.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class SipSecurityEvent
{
public:
   UtlString           m_sSRTPkey;
   void*               m_pCertificate;
   size_t              m_nCertificateSize;
   SIPX_SECURITY_EVENT m_Event;
   SIPX_SECURITY_CAUSE m_Cause;
   UtlString           m_sSubjAltName;
   UtlString           m_SessionCallId;
   UtlString           m_sRemoteAddress;
};


/**
* Listener for Security events
*/
class SipSecurityEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   SipSecurityEventListener() {}
   virtual ~SipSecurityEventListener() {}

   /* ============================ MANIPULATORS ============================== */

   virtual void OnEncrypt(const SipSecurityEvent& event) = 0;

   virtual void OnDecrypt(const SipSecurityEvent& event) = 0;

   virtual void OnTLS(const SipSecurityEvent& event) = 0;

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipSecurityEventListener_h__
