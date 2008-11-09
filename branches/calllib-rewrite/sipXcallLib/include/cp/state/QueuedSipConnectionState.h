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

#ifndef QueuedSipConnectionState_h__
#define QueuedSipConnectionState_h__

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
class QueuedSipConnectionState : public BaseSipConnectionState
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   QueuedSipConnectionState(XSipConnectionContext& rSipConnectionContext,
                            SipUserAgent& rSipUserAgent,
                            CpMediaInterfaceProvider* pMediaInterfaceProvider = NULL,
                            XSipConnectionEventSink* pSipConnectionEventSink = NULL);

   virtual ~QueuedSipConnectionState();

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

   virtual ISipConnectionState::StateEnum getCurrentState() { return ISipConnectionState::CONNECTION_QUEUED; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // QueuedSipConnectionState_h__
