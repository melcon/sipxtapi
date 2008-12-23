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

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/SipXInfoStatusEventListener.h"
#include <tapi/InfoStatusEventMsg.h>
#include "tapi/SipXEvents.h"
#include <tapi/SipXEventDispatcher.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

SipXInfoStatusEventListener::SipXInfoStatusEventListener(SIPX_INST pInst)
: OsSharedServerTask("SipXInfoStatusEventListener-%d")
, SipInfoStatusEventListener()
, m_pInst(pInst)
{

}

SipXInfoStatusEventListener::~SipXInfoStatusEventListener()
{
   release();
}

/* ============================ MANIPULATORS ============================== */

void SipXInfoStatusEventListener::OnResponse(const SipInfoStatusEvent& event)
{
   InfoStatusEventMsg msg(INFOSTATUS_RESPONSE, event);
   postMessage(msg);
}

void SipXInfoStatusEventListener::OnNetworkError(const SipInfoStatusEvent& event)
{
   InfoStatusEventMsg msg(INFOSTATUS_NETWORK_ERROR, event);
   postMessage(msg);
}

UtlBoolean SipXInfoStatusEventListener::handleMessage(OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;

   switch (rRawMsg.getMsgType())
   {
   case INFOSTATUS_MSG:
      {
         InfoStatusEventMsg* pMsg = dynamic_cast<InfoStatusEventMsg*>(&rRawMsg);
         if (pMsg)
         {
            // cast succeeded
            const SipInfoStatusEvent& payload = pMsg->getEventPayloadRef();
            handleInfoStatusEvent((SIPX_MESSAGE_STATUS)payload.m_status,
               payload.m_iResponseCode, payload.m_sResponseText, pMsg->getEvent(), payload.m_pCookie);
         }
      }
      bResult = TRUE;
      break;
   default:
      break;
   }
   return bResult;

}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void SipXInfoStatusEventListener::handleInfoStatusEvent(SIPX_MESSAGE_STATUS status,
                                                        int responseCode,
                                                        const UtlString& sResponseText,
                                                        SIPX_INFOSTATUS_EVENT event,
                                                        void* pCookie)
{
   SIPX_INFOSTATUS_INFO infoStatus;
   memset((void*)&infoStatus, 0, sizeof(SIPX_INFOSTATUS_INFO));

   infoStatus.nSize = sizeof(SIPX_INFOSTATUS_INFO);
   infoStatus.status = status;
   infoStatus.responseCode = responseCode;
   infoStatus.szResponseText = sResponseText.data();
   infoStatus.event = event;
   infoStatus.pCookie = pCookie;

   SipXEventDispatcher::dispatchEvent(m_pInst, EVENT_CATEGORY_INFO_STATUS, &infoStatus);
}

/* ============================ FUNCTIONS ================================= */

