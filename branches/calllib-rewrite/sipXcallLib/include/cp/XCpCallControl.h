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

#ifndef XCpCallControl_h__
#define XCpCallControl_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlDefs.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * XCpCallControl is meant to be used by calls/connections to control other calls.
 * This is needed in consultative call transfer.
 *
 * Methods of this interface must be executed from XCpAbstractCall thread, otherwise
 * deadlock might occur.
 */
class XCpCallControl
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /* ============================ MANIPULATORS ============================== */

   /** Attempts to drop connection of some abstract call, for given sip dialog */
   virtual OsStatus dropAbstractCallConnection(const SipDialog& sSipDialog) = 0;

   /* ============================ ACCESSORS ================================= */

   /** Checks if given call exists and is established. */
   virtual UtlBoolean isCallEstablished(const SipDialog& sipDialog) const = 0;

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // XCpCallControl_h__
