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

   /**
    * Sends message to specified call connection. If connection cannot be found,
    * OS_NOT_FOUND will be returned, and no more messages should be sent to the same
    * destination.
    */
   virtual OsStatus sendMessage(const OsMsg& msg, const SipDialog& sSipDialog) = 0;

   /**
    * Subscribe for given notification type with given target sip call.
    * ScNotificationMsg messages will be sent to callbackSipDialog.
    */
   virtual OsStatus subscribe(CP_NOTIFICATION_TYPE notificationType,
                              const SipDialog& targetSipDialog,
                              const SipDialog& callbackSipDialog) = 0;

   /**
    * Unsubscribes for given notification type with given target sip call.
    */
   virtual OsStatus unsubscribe(CP_NOTIFICATION_TYPE notificationType,
                                const SipDialog& targetSipDialog,
                                const SipDialog& callbackSipDialog) = 0;

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
