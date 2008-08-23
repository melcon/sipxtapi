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
   Url m_lineURI; ///< URI of line. Doesn't contain any parameters.
   SIPX_INSTANCE_DATA* m_pInst;
   OsMutex m_mutex;    
   SIPX_CONTACT_TYPE m_contactType; ///< contact type used by line
   UtlString m_contactIp; ///< IP of sip contact used by line
   int m_contactPort; ///< port of sip contact used by line
   SIPX_TRANSPORT_TYPE m_transport; ///< actual transport used for line registration
   UtlSList m_lineAliases;

   SIPX_LINE_DATA()
      : m_mutex(OsMutex::Q_FIFO)
      , m_lineURI(NULL)
      , m_pInst(NULL)
      , m_contactType(CONTACT_AUTO)
      , m_lineAliases()
      , m_contactIp(NULL)
      , m_contactPort(0)
      , m_transport(TRANSPORT_UDP)
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
