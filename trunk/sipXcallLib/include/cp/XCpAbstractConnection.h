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

#ifndef XCpAbstractConnection_h__
#define XCpAbstractConnection_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsMutex.h>
#include <os/OsSyncBase.h>
#include <utl/UtlContainable.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * XCpAbstractConnection is the top class for XCpSipConnection and any future subclasses
 * providing common functionality.
 */
class XCpAbstractConnection : public UtlContainable, public OsSyncBase
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   static const UtlContainableType TYPE;   /** < Class type used for runtime checking */ 

   /* ============================ CREATORS ================================== */

   XCpAbstractConnection();

   virtual ~XCpAbstractConnection();

   /* ============================ MANIPULATORS ============================== */

   /** Block until the sync object is acquired or the timeout expires */
   virtual OsStatus acquire(const OsTime& rTimeout = OsTime::OS_INFINITY);

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

   /* ============================ INQUIRY =================================== */

   /**
   * Compare the this object to another like-objects.  Results for 
   * designating a non-like object are undefined.
   *
   * @returns 0 if equal, < 0 if less then and >0 if greater.
   */
   virtual int compareTo(UtlContainable const* inVal) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   OsMutex m_memberMutex; ///< mutex for member synchronization
};

#endif // XCpAbstractConnection_h__
