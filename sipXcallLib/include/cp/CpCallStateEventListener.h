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

#ifndef CpCallStateEventListener_h__
#define CpCallStateEventListener_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include <net/SipSession.h>
#include "cp/CpCallLibEvents.h"

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

class CpCallStateEvent
{
public:

   UtlString m_sSessionCallId;
   UtlString m_sCallId;
   SipSession m_Session;
   UtlString m_sRemoteAddress;
   CP_CALLSTATE_CAUSE m_cause;
   const void* m_pEventData;
   int m_sipResponseCode;
   UtlString m_sResponseText;

   CpCallStateEvent(const UtlString& sSessionCallId,
                     const UtlString& sCallId,
                     SipSession& session,
                     const UtlString& sRemoteAddress,
                     CP_CALLSTATE_CAUSE cause,
                     const void* pEventData = NULL,
                     int sipResponseCode = 0,
                     const UtlString& sResponseText = NULL)
 : m_sSessionCallId(sSessionCallId),
   m_sCallId(sCallId),
   m_Session(session),
   m_sRemoteAddress(sRemoteAddress),
   m_cause(cause),
   m_pEventData(pEventData),
   m_sipResponseCode(sipResponseCode),
   m_sResponseText(sResponseText)
   {

   }

   CpCallStateEvent()
   : m_sipResponseCode(0)
   , m_pEventData(NULL)
   , m_cause(CP_CALLSTATE_CAUSE_UNKNOWN)
   {
   }
};


/**
* Listener for Call state events
*/
class CpCallStateEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   CpCallStateEventListener() {}
   virtual ~CpCallStateEventListener() {}

   /* ============================ MANIPULATORS ============================== */

   virtual void OnNewCall(const CpCallStateEvent& event) = 0;

   virtual void OnDialTone(const CpCallStateEvent& event) = 0;

   virtual void OnRemoteOffering(const CpCallStateEvent& event) = 0;

   virtual void OnRemoteAlerting(const CpCallStateEvent& event) = 0;

   virtual void OnConnected(const CpCallStateEvent& event) = 0;

   virtual void OnBridged(const CpCallStateEvent& event) = 0;

   virtual void OnHeld(const CpCallStateEvent& event) = 0;

   virtual void OnRemoteHeld(const CpCallStateEvent& event) = 0;

   virtual void OnDisconnected(const CpCallStateEvent& event) = 0;

   virtual void OnOffering(const CpCallStateEvent& event) = 0;

   virtual void OnAlerting(const CpCallStateEvent& event) = 0;

   virtual void OnDestroyed(const CpCallStateEvent& event) = 0;

   virtual void OnTransferEvent(const CpCallStateEvent& event) = 0;

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // CpCallStateEventListener_h__
