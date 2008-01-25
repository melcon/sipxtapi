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

#ifndef SipLineStateEventListener_h__
#define SipLineStateEventListener_h__

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

class SipLineStateEvent
{
public:

   UtlString m_sLineId;
   SIPX_LINESTATE_CAUSE m_Cause;
   int m_responseCode;
   UtlString m_sResponseText;

   SipLineStateEvent(const UtlString& lineId, SIPX_LINESTATE_CAUSE cause, int responseCode = 0, const UtlString& sResponseText = NULL)
      : m_sLineId(lineId),
        m_Cause(cause),
        m_responseCode(responseCode),
        m_sResponseText(sResponseText)
   {

   }
};


/**
* Listener for Line state events
*/
class SipLineStateEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   SipLineStateEventListener() {}
   virtual ~SipLineStateEventListener() {}

   /* ============================ MANIPULATORS ============================== */

   virtual void OnLineRegistering(const SipLineStateEvent& event) = 0;

   virtual void OnLineRegistered(const SipLineStateEvent& event) = 0;

   virtual void OnLineUnregistering(const SipLineStateEvent& event) = 0;

   virtual void OnLineUnregistered(const SipLineStateEvent& event) = 0;

   virtual void OnLineRegisterFailed(const SipLineStateEvent& event) = 0;

   virtual void OnLineUnregisterFailed(const SipLineStateEvent& event) = 0;

   virtual void OnLineProvisioned(const SipLineStateEvent& event) = 0;

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipLineStateEventListener_h__
