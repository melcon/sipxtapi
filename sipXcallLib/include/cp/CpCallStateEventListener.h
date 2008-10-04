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
#include "tapi/sipXtapiEvents.h"
#include <net/SipSession.h>

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
   SIPX_CALLSTATE_CAUSE m_cause;
   UtlString m_sOriginalSessionCallId; // callId supplied sometimes for transfer events
   int m_sipResponseCode;
   UtlString m_sResponseText;

   CpCallStateEvent(const UtlString& sSessionCallId,
                     const UtlString& sCallId,
                     const SipSession& session,
                     const UtlString& sRemoteAddress,
                     SIPX_CALLSTATE_CAUSE cause,
                     const UtlString& sOriginalSessionCallId = NULL,
                     int sipResponseCode = 0,
                     const UtlString& sResponseText = NULL)
 : m_sSessionCallId(sSessionCallId),
   m_sCallId(sCallId),
   m_Session(session),
   m_sRemoteAddress(sRemoteAddress),
   m_cause(cause),
   m_sOriginalSessionCallId(sOriginalSessionCallId),
   m_sipResponseCode(sipResponseCode),
   m_sResponseText(sResponseText)
   {

   }

   CpCallStateEvent()
   {
      m_sipResponseCode = 0;
      m_cause = CALLSTATE_CAUSE_UNKNOWN;
   }

   ~CpCallStateEvent()
   {
   }

   CpCallStateEvent(const CpCallStateEvent& event)
   {
      *this = event;
   }

   CpCallStateEvent& operator=(const CpCallStateEvent& event)
   {
      if (&event == this)
      {
         return *this;
      }

      m_sCallId = event.m_sCallId;
      m_sSessionCallId = event.m_sSessionCallId;
      m_Session = event.m_Session;
      m_sRemoteAddress = event.m_sRemoteAddress;
      m_cause = event.m_cause;
      m_sOriginalSessionCallId = event.m_sOriginalSessionCallId;
      m_sipResponseCode = event.m_sipResponseCode;
      m_sResponseText = event.m_sResponseText;
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
