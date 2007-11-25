//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
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

#ifndef SipXInfoStatusEventListener_h__
#define SipXInfoStatusEventListener_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/sipXtapi.h"
#include "net/SipInfoStatusEventListener.h"

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


/**
* Listener for Info events
*/
class SipXInfoStatusEventListener : public SipInfoStatusEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   SipXInfoStatusEventListener(SIPX_INST pInst);
   virtual ~SipXInfoStatusEventListener();

   /* ============================ MANIPULATORS ============================== */

   virtual void OnResponse(const SipInfoStatusEvent& event);

   virtual void OnNetworkError(const SipInfoStatusEvent& event);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   SIPX_INST m_pInst;

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipXInfoStatusEventListener_h__
