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

#ifndef SipLineProvider_h__
#define SipLineProvider_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * Interface describing basic functionality definition of a sip line provider.
 */
class SipLineProvider
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /**
   * Tries to find line according to given parameters. First try lookup by
   * lineId if its supplied. If lineId is not supplied, lookup by identityUri. If
   * not found by identityUri, try by userId.
   */
   virtual UtlBoolean lineExists(const UtlString& lineId,
                                 const Url& lineUri,
                                 const UtlString& userId) const = 0;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipLineProvider_h__
