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
#include <cp/state/SipConnectionStateMachine.h>
#include <cp/state/SipConnectionStateObserver.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class CpMediaInterfaceProvider;
class SipUserAgent;

/**
 * XSipConnection is responsible for SIP communication.
 *
 * All manipulators except OsSyncBase methods must be called from single thread only.
 * Inquiry and accessor methods may be called from multiple threads.
 */
class XSipConnection : public UtlContainable, public OsSyncBase, public SipConnectionStateObserver
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   static const UtlContainableType TYPE;   /** < Class type used for runtime checking */ 

   /* ============================ CREATORS ================================== */

   XSipConnection(SipUserAgent& rSipUserAgent,
                  CpMediaInterfaceProvider* pMediaInterfaceProvider = NULL);

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

   /**
   * Called when we enter new state. This is typically called after we handle
   * SipMessageEvent, resulting in new state transition.
   */
   virtual void handleStateEntry(ISipConnectionState::StateEnum state);

   /**
   * Called when we progress to new state, before old state is destroyed.
   */
   virtual void handleStateExit(ISipConnectionState::StateEnum state);

   // needs special locking
   mutable XSipConnectionContext m_sipConnectionContext; ///< contains stateful information about sip connection.
   // not thread safe, must be used from single thread only
   SipConnectionStateMachine m_stateMachine; ///< state machine for handling commands and SipMessageEvents
   // thread safe
   SipUserAgent& m_rSipUserAgent; // for sending sip messages
   CpMediaInterfaceProvider* m_pMediaInterfaceProvider; ///< media interface provider

   mutable OsRWMutex m_instanceRWMutex; ///< mutex for guarding instance against deletion from XCpAbstractCall
};

#endif // XSipConnection_h__
