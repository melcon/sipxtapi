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

#ifndef ISipConnectionState_h__
#define ISipConnectionState_h__

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
class SipMessageEvent;

/**
 * Interface for all connection states.
 */
class ISipConnectionState
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /**
    * Codes for possible connection states.
    */
   typedef enum
   {
      CONNECTION_IDLE = 0,
      CONNECTION_QUEUED,
      CONNECTION_OFFERING,
      CONNECTION_ALERTING,
      CONNECTION_ESTABLISHED,
      CONNECTION_FAILED,
      CONNECTION_DISCONNECTED,
      CONNECTION_UNKNOWN,
      CONNECTION_INITIATED,
      CONNECTION_DIALING,
      CONNECTION_NETWORK_REACHED,
      CONNECTION_NETWORK_ALERTING
   } StateEnum;

   /* ============================ CREATORS ================================== */

   /* ============================ MANIPULATORS ============================== */

   /**
    * Called when we enter new state. This is typically called after we handle
    * SipMessageEvent, resulting in new state transition. Should not be used to
    * send any sip messages.
    */
   virtual void handleStateEntry() = 0;

   /**
    * Called before this state is destroyed. Should not be used to send any sip messages.
    */
   virtual void handleStateExit() = 0;

   /**
    * Handles SipMessageEvent, which can be inbound sip request/response or error
    * sending status.
    * If instance cannot handle this event, it must pass it to parent as the last resort.
    *
    * @param rEvent Instance of SipMessageEvent that needs to be handled.
    * @return New state to progress into or this if no state progression is made.
    */
   virtual ISipConnectionState* handleSipMessageEvent(const SipMessageEvent& rEvent) = 0;

   /* ============================ ACCESSORS ================================= */

   /**
    * Gets id of current state.
    */
   virtual ISipConnectionState::StateEnum getCurrentState() = 0;

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // ISipConnectionState_h__
