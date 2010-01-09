//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/SipXMessageObserver.h"
#include <tapi/SipXCore.h>
#include "tapi/SipXEvents.h"
#include "tapi/SipXEventDispatcher.h"
#include "net/SipUserAgent.h"
#include "os/OsEventMsg.h"
#include <os/OsStunResultFailureMsg.h>
#include <os/OsStunResultSuccessMsg.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */


SipXMessageObserver::SipXMessageObserver(SIPX_INSTANCE_DATA* pInst)
: OsServerTask("SipXMessageObserver%d", NULL, 2000)
, m_pInst(pInst)
{
}


SipXMessageObserver::~SipXMessageObserver(void)
{
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */


UtlBoolean SipXMessageObserver::handleMessage(OsMsg& rMsg)
{
   UtlBoolean bRet = FALSE;
   unsigned char msgType = rMsg.getMsgType();
   unsigned char msgSubType = rMsg.getMsgSubType();

   // Stun event notification
   if (msgType == OsMsg::OS_STUN_RESULT_MSG)
   {
      const OsStunResultMsg* pResultMsg = dynamic_cast<const OsStunResultMsg*>(&rMsg);
      if (pResultMsg)
      {
         return handleStunOutcome(*pResultMsg);
      }
   }
   return bRet;
}

UtlBoolean SipXMessageObserver::handleStunOutcome(const OsStunResultMsg& pResultMsg) 
{
   switch(pResultMsg.getMsgSubType())
   {
      case OsStunResultMsg::STUN_RESULT_SUCCESS:
         {
            const OsStunResultSuccessMsg* pResultSuccessMsg = dynamic_cast<const OsStunResultSuccessMsg*>(&pResultMsg);
            if (pResultSuccessMsg)
            {
               return handleStunSuccess(*pResultSuccessMsg);
            }
            break;
         }
      case OsStunResultMsg::STUN_RESULT_FAILURE:
         {
            const OsStunResultFailureMsg* pResultFailureMsg = dynamic_cast<const OsStunResultFailureMsg*>(&pResultMsg);
            if (pResultFailureMsg)
            {
               return handleStunFailure(*pResultFailureMsg);
            }
            break;
         }
      default:
         ;
   }

   return TRUE;
}

UtlBoolean SipXMessageObserver::handleStunSuccess(const OsStunResultSuccessMsg& pResultMsg)
{
   UtlString adapterName;
   UtlString mappedIp;
   int mappedPort;
   pResultMsg.getAdapterName(adapterName);
   pResultMsg.getMappedIp(mappedIp);
   mappedPort = pResultMsg.getMappedPort();
   // create SIPX_CONTACT_ADDRESS
   SIPX_CONTACT_ADDRESS stunContact;
   stunContact.id = 0; // will be assigned by SipUserAgent
   SAFE_STRNCPY(stunContact.cInterface, adapterName.data(), sizeof(stunContact.cInterface));
   SAFE_STRNCPY(stunContact.cIpAddress, mappedIp.data(), sizeof(stunContact.cIpAddress));
   stunContact.iPort = mappedPort;
   stunContact.eContactType = CONTACT_NAT_MAPPED;
   stunContact.eTransportType = TRANSPORT_UDP;
   // first, find the user-agent, and add the contact to
   // the user-agent's db
   m_pInst->pSipUserAgent->addContactAddress(stunContact);
   sipxFireConfigEvent(m_pInst, CONFIG_STUN_SUCCESS, &stunContact);

   return TRUE;
}

UtlBoolean SipXMessageObserver::handleStunFailure(const OsStunResultFailureMsg& pResultMsg)
{
   sipxFireConfigEvent(m_pInst, CONFIG_STUN_FAILURE, NULL);

   return TRUE;
}
