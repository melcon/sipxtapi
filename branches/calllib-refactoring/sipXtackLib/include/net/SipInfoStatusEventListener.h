//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef SipInfoStatusEventListener_h__
#define SipInfoStatusEventListener_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include "tapi/sipXtapiEvents.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class SipInfoStatusEvent
{
public:
   SIPX_INFOSTATUS_EVENT m_Event;
   SIPX_MESSAGE_STATUS m_Status;
   int m_iResponseCode;
   UtlString m_sResponseText;  
};


/**
* Listener for Info events
*/
class SipInfoStatusEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   SipInfoStatusEventListener() {}
   virtual ~SipInfoStatusEventListener() {}

   /* ============================ MANIPULATORS ============================== */

   virtual void OnResponse(const SipInfoStatusEvent& event) = 0;

   virtual void OnNetworkError(const SipInfoStatusEvent& event) = 0;

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipInfoStatusEventListener_h__
