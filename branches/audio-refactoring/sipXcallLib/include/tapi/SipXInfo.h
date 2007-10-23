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

#ifndef SipXInfo_h__
#define SipXInfo_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsRWMutex.h>
#include "tapi/SipXCore.h"
#include "tapi/sipXtapiEvents.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
class SIPX_INSTANCE_DATA;

// STRUCTS
// TYPEDEFS
class SIPX_INFO_DATA
{
public:
   SIPX_INFO_INFO infoData;
   SIPX_INSTANCE_DATA* pInst;
   OsMutex mutex;

   SIPX_INFO_DATA() : pInst(NULL),
      mutex(OsMutex::Q_FIFO)
   {
      memset(&infoData, 0, sizeof(SIPX_INFO_INFO));
   }
};

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

/**
 * Looks up the SIPX_INFO_DATA structure pointer, given the SIPX_INFO handle.
 * @param hInfo Info Handle
 * @param type Lock type to use during lookup.
 */
SIPX_INFO_DATA* sipxInfoLookup(const SIPX_INFO hInfo,
                               SIPX_LOCK_TYPE type,
                               const OsStackTraceLogger& oneBackInStack);

/**
 * Unlocks the mutex associated with the INFO DATA
 * 
 * @param pData pointer to the SIPX_INFO structure
 * @param type Type of lock (read or write)
 */
void sipxInfoReleaseLock(SIPX_INFO_DATA* pData,
                         SIPX_LOCK_TYPE type,
                         const OsStackTraceLogger& oneBackInStack) ;

/**
 * Releases the INFO handle created by a call to sipxCallSendInfo.
 * Also calls sipxInfoFree.
 *
 * @param hInfo Handle to the Info object
 */
void sipxInfoObjectFree(SIPX_INFO hInfo);

/**
 * Frees the INFO structure allocated by a call to sipxCallSendInfo
 *
 * @param pData Pointer to SIPX_INFO_DATA structure
 */
void sipxInfoFree(SIPX_INFO_DATA* pData);

#endif // SipXInfo_h__
