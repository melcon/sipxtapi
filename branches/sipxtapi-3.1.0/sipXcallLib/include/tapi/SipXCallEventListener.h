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


#ifndef SipXCallEventListener_h__
#define SipXCallEventListener_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/sipXtapi.h"
#include <cp/CpCallStateEventListener.h>

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
* Listener for Call state events
*/
class SipXCallEventListener : public CpCallStateEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   SipXCallEventListener(SIPX_INST pInst);
   virtual ~SipXCallEventListener();

   /* ============================ MANIPULATORS ============================== */

   virtual void OnNewCall(const CpCallStateEvent& event);

   virtual void OnDialTone(const CpCallStateEvent& event);

   virtual void OnRemoteOffering(const CpCallStateEvent& event);

   virtual void OnRemoteAlerting(const CpCallStateEvent& event);

   virtual void OnConnected(const CpCallStateEvent& event);

   virtual void OnBridged(const CpCallStateEvent& event);

   virtual void OnHeld(const CpCallStateEvent& event);

   virtual void OnRemoteHeld(const CpCallStateEvent& event);

   virtual void OnDisconnected(const CpCallStateEvent& event);

   virtual void OnOffering(const CpCallStateEvent& event);

   virtual void OnAlerting(const CpCallStateEvent& event);

   virtual void OnDestroyed(const CpCallStateEvent& event);

   virtual void OnTransferEvent(const CpCallStateEvent& event);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   SIPX_INST m_pInst;

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipXCallEventListener_h__
