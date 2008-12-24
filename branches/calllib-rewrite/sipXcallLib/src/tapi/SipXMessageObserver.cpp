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
    m_iTestResponseCode(0),// if mTestResponseCode is set to a value other than 0,
                         // then this message observer can generate a test response.
                         // This feature is used by sipXtapiTest
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
       UtlString method;

       pSipMessage->getRequestMethod(&method);

       if (pSipMessage)
       {
          if (pSipMessage->isRequest() && method.compareTo(SIP_INFO_METHOD))
          {
             // ok, the phone has received an INFO message.
             bRet = handleIncomingInfoMessage(pSipMessage);
          }
       }
    }
    return bRet;
}

UtlBoolean SipXMessageObserver::handleIncomingInfoMessage(SipMessage* pMessage)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "SipXMessageObserver::handleIncomingInfoMessage");
    bool bRet = FALSE;
    SIPX_INSTANCE_DATA* pInst = SAFE_PTR_CAST(SIPX_INSTANCE_DATA, pMessage->getResponseListenerData());
    assert(pInst);
    
    if (pInst && pMessage)
    {
       if (m_iTestResponseCode != 0)  // for unit testing purposes.
       {
           if (m_iTestResponseCode == SIP_REQUEST_TIMEOUT_CODE)   // a timeout response is being tested
           {
               // simulate a timeout ....
               OsTask::delay(1000);
               // respond to whomever sent us the message
	           SipMessage sipResponse;
	           sipResponse.setOkResponseData(pMessage);
              sipResponse.setResponseData(pMessage, m_iTestResponseCode, "timed out");	       
	           pInst->pSipUserAgent->send(sipResponse);
              return TRUE;
           }
       }
       else
       {
            // respond to whomever sent us the message
	        SipMessage sipResponse;
	        sipResponse.setOkResponseData(pMessage);
	        pInst->pSipUserAgent->send(sipResponse);
	    }
	    
       // Find Line
       UtlString lineUri;
       pMessage->getToUri(&lineUri);

       UtlString requestUri;
       pMessage->getRequestUri(&requestUri);

       SIPX_INSTANCE_DATA* pInst = SAFE_PTR_CAST(SIPX_INSTANCE_DATA, m_hInst);
       SIPX_LINE hLine = sipxLineLookupHandle(pInst, lineUri, requestUri);
        
        if (!pMessage->isResponse())
        {
            // find call
            UtlString callId;
            pMessage->getCallIdField(&callId);

            SIPX_CALL hCall = sipxCallLookupHandleBySessionCallId(callId, pInst);

            if (hCall == 0)
            {
                // we are unaware of the call context
            }
            
            SIPX_INFO_INFO pInfoInfo;
            memset((void*)&pInfoInfo, 0, sizeof(SIPX_INFO_INFO));
            
            pInfoInfo.nSize = sizeof(SIPX_INFO_INFO);
            pInfoInfo.hCall = hCall;
            pInfoInfo.hLine = hLine;
            Url fromUrl;
            
            // passing pointer to UtlString buffer is safe here
            pInfoInfo.szFromURL = lineUri.data();

            // get the user agent
            UtlString userAgent;
            pMessage->getUserAgentField(&userAgent);
            // passing pointer to UtlString buffer is safe here
            pInfoInfo.szUserAgent = userAgent.data();

            // get and set the content type
            UtlString contentType;
            pMessage->getContentType(&contentType) ;
            // passing pointer to UtlString buffer is safe here
            pInfoInfo.szContentType = contentType.data();
            
            // get the content
            UtlString body;
            int dummyLength = pMessage->getContentLength();
            const HttpBody* pBody = pMessage->getBody();
            pBody->getBytes(&body, &dummyLength);
            // passing pointer to UtlString buffer is safe here
            pInfoInfo.pContent = body.data();
            pInfoInfo.nContentLength = pMessage->getContentLength();

            // dispatcher doesn't delete this event
            SipXEventDispatcher::dispatchEvent(pInst, EVENT_CATEGORY_INFO, &pInfoInfo);

            bRet = TRUE;
        }
    } // if (NULL != pInst && NULL != pMessage)
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
