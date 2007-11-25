//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _SipXMessageObserver_h_
#define _SipXMessageObserver_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "os/OsServerTask.h"
#include "net/SipMessage.h"
#include "net/SipMessageEvent.h"
#include "sipXtapi.h"


// DEFINES
#define SIPXMO_NOTIFICATION_STUN    1
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS

// FORWARD DECLARATIONS
class OsEventMsg;

/**
 *  Class that is an OsServerTask, and has a message queue that observes SIP messages.
 *  For example, it is used for looking for message responses, like the INFO response.
 */
class SipXMessageObserver : public OsServerTask
{
public:
/* ============================ CREATORS ================================== */

    SipXMessageObserver(const SIPX_INST hInst);
    virtual ~SipXMessageObserver(void);
    
/* ============================ MANIPULATORS ============================== */

    /**
     * Implementation of OsServerTask's pure virtual method
     */
    UtlBoolean handleMessage(OsMsg& rMsg);
    
    /**
     * FOR TEST PURPOSES ONLY - a response code to send back to the client
     */
    void setTestResponseCode(int code) { m_iTestResponseCode = code; }    
    
private:
    UtlBoolean handleIncomingInfoMessage(SipMessage* pMessage);
    UtlBoolean handleIncomingInfoStatus(SipMessage* pMessage);
    UtlBoolean handleStunOutcome(OsEventMsg* pMsg);

    /** 
     * Special response code - for test purposes only.
     */
    int m_iTestResponseCode;    
    SIPX_INST m_hInst;
};

#endif
