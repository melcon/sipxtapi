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

#ifndef XSipConnection_h__
#define XSipConnection_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsMutex.h>
#include <os/OsRWMutex.h>
#include <os/OsSyncBase.h>
#include <utl/UtlContainable.h>
#include <cp/XSipConnectionContext.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * XSipConnection is responsible for SIP communication.
 */
class XSipConnection : public UtlContainable, public OsSyncBase
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   static const UtlContainableType TYPE;   /** < Class type used for runtime checking */ 

   /* ============================ CREATORS ================================== */

   XSipConnection();

   virtual ~XSipConnection();

   /* ============================ MANIPULATORS ============================== */

   /** Block until the sync object is acquired. Timeout is not supported! */
   virtual OsStatus acquire(const OsTime& rTimeout = OsTime::OS_INFINITY);

   /** Acquires exclusive lock on instance. Use only when deleting. It is never released. */
   virtual OsStatus acquireExclusive();

   /** Conditionally acquire the semaphore (i.e., don't block) */
   virtual OsStatus tryAcquire();

   /** Release the sync object */
   virtual OsStatus release();

   /* ============================ ACCESSORS ================================= */

   /**
   * Calculate a unique hash code for this object.  If the equals
   * operator returns true for another object, then both of those
   * objects must return the same hashcode.
   */    
   virtual unsigned hash() const;

   /**
   * Get the ContainableType for a UtlContainable derived class.
   */
   virtual UtlContainableType getContainableType() const;

   /**
   * Gets a copy of connection sip dialog.
   */
   void getSipDialog(SipDialog& sSipDialog) const;

   /**
    * Gets sip call-id of the connection.
    */
   void getSipCallId(UtlString& sSipCallId) const;

   /**
    * Gets user agent of the remote party if known.
    */
   void getRemoteUserAgent(UtlString& sRemoteUserAgent) const;

   /** Gets internal id of media connection for connection. Only for unit tests */
   void getMediaConnectionId(int& mediaConnID) const;

   /* ============================ INQUIRY =================================== */

   /**
   * Compare the this object to another like-objects.  Results for 
   * designating a non-like object are undefined.
   *
   * @returns 0 if equal, < 0 if less then and >0 if greater.
   */
   virtual int compareTo(UtlContainable const* inVal) const;

   /**
   * Checks if this call has given sip dialog.
   */
   SipDialog::DialogMatchEnum compareSipDialog(const SipDialog& sSipDialog) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XSipConnection(const XSipConnection& rhs);

   XSipConnection& operator=(const XSipConnection& rhs);

   // needs special locking
   mutable XSipConnectionContext m_sipConnectionContext; ///< contains stateful information about sip connection.

   mutable OsRWMutex m_instanceRWMutex; ///< mutex for guarding instance against deletion from XCpAbstractCall
};

#endif // XSipConnection_h__
