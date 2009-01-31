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
#include <tapi/SipXInfoStatusEventListener.h>
#include "tapi/sipXtapi.h"
#include "tapi/sipXtapiEvents.h"
#include "tapi/SipXEvents.h"
#include "tapi/SipXHandleMap.h"
#include "tapi/SipXCall.h"
#include "tapi/SipXLine.h"
#include "tapi/SipXInfo.h"
#include "tapi/SipXEventDispatcher.h"
#include "net/SipUserAgent.h"
#include "utl/UtlVoidPtr.h"
#include "os/OsEventMsg.h"
#include "os/OsLock.h"
#include "os/OsTimer.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */


SipXMessageObserver::SipXMessageObserver(const SIPX_INST hInst) :
    OsServerTask("SipXMessageObserver%d", NULL, 2000),
    m_hInst(hInst)
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

    // Queued event notification
    if (msgType == OsMsg::OS_EVENT)
    {
        OsEventMsg* pEventMsg = (OsEventMsg*)&rMsg;
        int eventType;
        pEventMsg->getUserData(eventType);

        // fine select by event user data
        switch (eventType)
        {
            case SIPXMO_NOTIFICATION_STUN:
                handleStunOutcome(pEventMsg);
                bRet = TRUE;
                break;
            default:
               // this shouldn't be used at all
               assert(false);
               break;
        }                
    }
    else if (msgType == OsMsg::PHONE_APP && msgSubType == SipMessage::NET_SIP_MESSAGE)
    {
       SipMessage* pSipMessage = (SipMessage*)((SipMessageEvent&)rMsg).getMessage();
       // add handlers for any other messages
    }
    return bRet;
}

UtlBoolean SipXMessageObserver::handleStunOutcome(OsEventMsg* pMsg) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "SipXMessageObserver::handleStunOutcome");
   SIPX_INSTANCE_DATA* pInst = SAFE_PTR_CAST(SIPX_INSTANCE_DATA, m_hInst);
   SIPX_CONTACT_ADDRESS* pStunContact = NULL;
   pMsg->getEventData((int&)pStunContact);

    if (pStunContact)
    {
        // first, find the user-agent, and add the contact to
        // the user-agent's db
        assert(pInst);
        pInst->pSipUserAgent->addContactAddress(*pStunContact);
        sipxFireConfigEvent(pInst, CONFIG_STUN_SUCCESS, pStunContact);

        // If we have an external transport, also create a record for the 
        // external transport
        SIPX_CONTACT_ADDRESS externalTransportContact;

        // TODO: At the point where we support multiple external 
        // transports, this code needs to iterate through ALL of
        // the external transports.

        // ????? VERIFY
        if (pInst->pSipUserAgent->getContactDb().getRecordForAdapter(externalTransportContact,
                                                                     pStunContact->cInterface,
                                                                     CONTACT_LOCAL,
                                                                     TRANSPORT_CUSTOM))
        {
            SIPX_CONTACT_ADDRESS* pExtContact = NULL;
            pExtContact = new SIPX_CONTACT_ADDRESS(externalTransportContact);
            pExtContact->eContactType = CONTACT_NAT_MAPPED;
            pExtContact->id = 0;
            SAFE_STRNCPY(pExtContact->cIpAddress, pStunContact->cIpAddress, sizeof(pExtContact->cIpAddress));
            pExtContact->iPort = pStunContact->iPort;
            pInst->pSipUserAgent->addContactAddress(*pExtContact);
            sipxFireConfigEvent(pInst, CONFIG_STUN_SUCCESS, pExtContact);
            delete pExtContact;
        }
        
        delete pStunContact;
    }
    else
    {
       sipxFireConfigEvent(pInst, CONFIG_STUN_FAILURE, NULL);
    }

    return TRUE;
}
