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

#ifndef UnknownSipConnectionState_h__
#define UnknownSipConnectionState_h__

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
 * State representing unknown connection state.
 */
class UnknownSipConnectionState : public BaseSipConnectionState
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   UnknownSipConnectionState(XSipConnectionContext& rSipConnectionContext,
                             SipUserAgent& rSipUserAgent,
                             CpMediaInterfaceProvider* pMediaInterfaceProvider = NULL,
                             XSipConnectionEventSink* pSipConnectionEventSink = NULL);

   virtual ~UnknownSipConnectionState();

   /* ============================ MANIPULATORS ============================== */

   /**
   * State entry handler.
   */
   virtual void handleStateEntry(StateEnum previousState);

   /**
   * State exit handler.
   */
   virtual void handleStateExit(StateEnum nextState);

   virtual SipConnectionStateTransition* handleSipMessageEvent(const SipMessageEvent& rEvent);

   /* ============================ ACCESSORS ================================= */

   virtual ISipConnectionState::StateEnum getCurrentState() { return ISipConnectionState::CONNECTION_UNKNOWN; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // UnknownSipConnectionState_h__
