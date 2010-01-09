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
      
private:
    UtlBoolean handleStunOutcome(OsEventMsg* pMsg);

    SIPX_INST m_hInst;
};

#endif
