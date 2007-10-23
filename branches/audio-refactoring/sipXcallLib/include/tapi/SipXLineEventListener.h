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


#ifndef SipXLineEventListener_h__
#define SipXLineEventListener_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/sipXtapi.h"
#include <net/SipLineStateEventListener.h>

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
* Listener for Line state events
*/
class SipXLineEventListener : public SipLineStateEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   SipXLineEventListener(SIPX_INST pInst);
   virtual ~SipXLineEventListener();

   /* ============================ MANIPULATORS ============================== */

   virtual void OnLineRegistering(const SipLineStateEvent& event);

   virtual void OnLineRegistered(const SipLineStateEvent& event);

   virtual void OnLineUnregistering(const SipLineStateEvent& event);

   virtual void OnLineUnregistered(const SipLineStateEvent& event);

   virtual void OnLineRegisterFailed(const SipLineStateEvent& event);

   virtual void OnLineUnregisterFailed(const SipLineStateEvent& event);

   virtual void OnLineProvisioned(const SipLineStateEvent& event);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   SIPX_INST m_pInst;

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipXLineEventListener_h__
