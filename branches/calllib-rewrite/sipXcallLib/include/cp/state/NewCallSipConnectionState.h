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

#ifndef NewCallSipConnectionState_h__
#define NewCallSipConnectionState_h__

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
class NewCallSipConnectionState : public BaseSipConnectionState
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   NewCallSipConnectionState(XSipConnectionContext& rSipConnectionContext,
                             SipUserAgent& rSipUserAgent,
                             CpMediaInterfaceProvider* pMediaInterfaceProvider = NULL,
                             XSipConnectionEventSink* pSipConnectionEventSink = NULL);

   virtual ~NewCallSipConnectionState();

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

   virtual ISipConnectionState::StateEnum getCurrentState() { return ISipConnectionState::CONNECTION_NEWCALL; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // NewCallSipConnectionState_h__
