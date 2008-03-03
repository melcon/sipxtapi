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

#ifndef SipXLine_h__
#define SipXLine_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/SipXCore.h"
#include <net/Url.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS
class SIPX_LINE_DATA
{
public:
   Url lineURI;
   SIPX_INSTANCE_DATA* pInst;
   OsMutex             mutex;    
   SIPX_CONTACT_TYPE   contactType;
   UtlSList            lineAliases;
   SIPX_CONTACT_ID     contactId;

   SIPX_LINE_DATA()
      : mutex(OsMutex::Q_FIFO),
      lineURI(NULL),
      pInst(NULL),
      contactType(CONTACT_AUTO),
      lineAliases(),
      contactId(0)
   {
   }

};

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

SIPX_LINE_DATA* sipxLineLookup(const SIPX_LINE hLine,
                               SIPX_LOCK_TYPE type,
                               const OsStackTraceLogger& oneBackInStack);

SIPX_LINE sipxLineLookupHandle(const char* szLineURI, const char* requestUri);
SIPX_LINE sipxLineLookupHandleByURI(const char* szURI);

void sipxLineReleaseLock(SIPX_LINE_DATA* pData,
                         SIPX_LOCK_TYPE type,
                         const OsStackTraceLogger& oneBackInStack);

void sipxLineRemoveAll(const SIPX_INST hInst);

void sipxLineObjectFree(const SIPX_LINE hLine);

UtlBoolean validLineData(const SIPX_LINE_DATA* pData);

#endif // SipXLine_h__
