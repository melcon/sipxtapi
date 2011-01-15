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

#ifndef SipTransport_h__
#define SipTransport_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS

class Url;
class UtlString;

// STRUCTS
// TYPEDEFS

/**
 * SIPXSTACK_TRANSPORT_TYPE defines various protocols use for signaling 
 * transport.
 */
typedef enum
{
   SIPXSTACK_TRANSPORT_UDP = 1,  /**< Indicator for a UDP socket type. */
   SIPXSTACK_TRANSPORT_TCP = 0,  /**< Indicator for a TCP socket type. */ 
   SIPXSTACK_TRANSPORT_TLS = 3,  /**< Indicator for a TLS socket type. */
   SIPXSTACK_TRANSPORT_CUSTOM = 4 /**< Indicator for a custom external transport. */
} SIPXSTACK_TRANSPORT_TYPE;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class SipTransport
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /* ============================ MANIPULATORS ============================== */

   static SIPXSTACK_TRANSPORT_TYPE getSipTransport(const Url& url);
   static SIPXSTACK_TRANSPORT_TYPE getSipTransport(const UtlString& strUrl);
   static SIPXSTACK_TRANSPORT_TYPE getSipTransport(const char* szUrl);

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipTransport_h__
