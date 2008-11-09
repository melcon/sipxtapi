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

#ifndef FailedSipConnectionState_h__
#define FailedSipConnectionState_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <cp/state/BaseSipConnectionState.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * State representing idle connection.
 */
class FailedSipConnectionState : public BaseSipConnectionState
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   FailedSipConnectionState(XSipConnectionContext& rSipConnectionContext,
                            SipUserAgent& rSipUserAgent,
                            CpMediaInterfaceProvider* pMediaInterfaceProvider = NULL,
                            XSipConnectionEventSink* pSipConnectionEventSink = NULL);

   virtual ~FailedSipConnectionState();

   /* ============================ MANIPULATORS ============================== */

   /**
   * State entry handler.
   */
   virtual void handleStateEntry();

   /**
   * State exit handler.
   */
   virtual void handleStateExit();

   virtual ISipConnectionState* handleSipMessageEvent(const SipMessageEvent& rEvent);

   /* ============================ ACCESSORS ================================= */

   virtual ISipConnectionState::StateEnum getCurrentState() { return ISipConnectionState::CONNECTION_FAILED; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // FailedSipConnectionState_h__
