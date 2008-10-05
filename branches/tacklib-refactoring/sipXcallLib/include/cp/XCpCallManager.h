//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef XCallManager_h__
#define XCallManager_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include <os/OsMutex.h>
#include <os/OsServerTask.h>
#include <utl/UtlHashMap.h>
#include <net/SipCallIdGenerator.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * Call manager class. Responsible for creation of calls, management of calls via various operations, conferencing.
 */
class XCpCallManager : public OsServerTask
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   XCpCallManager(UtlBoolean doNotDisturb,
                  UtlBoolean bEnableICE,
                  int rtpPortStart,
                  int rtpPortEnd);

   virtual ~XCpCallManager();

   /* ============================ MANIPULATORS ============================== */

   virtual UtlBoolean handleMessage(OsMsg& rRawMsg);

   /* ============================ ACCESSORS ================================= */

   UtlBoolean getDoNotDisturb() const { return m_bDoNotDisturb; }
   void setDoNotDisturb(UtlBoolean val) { m_bDoNotDisturb = val; }

   UtlBoolean getEnableICE() const { return m_bEnableICE; }
   void setEnableICE(UtlBoolean val) { m_bEnableICE = val; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XCpCallManager(const XCpCallManager& rhs);

   XCpCallManager& operator=(const XCpCallManager& rhs);

   typedef enum
   {
      ID_TYPE_CALL,
      ID_TYPE_CONFERENCE,
      ID_TYPE_SIP
   } ID_TYPE;

   /** Generates new id for call. It is not the call-id field used in sip messages, instead its an internal id */
   UtlString getNewCallId();

   /** Generates new id for conference */
   UtlString getNewConferenceId();

   /** Generates new sip call-id */
   UtlString getNewSipCallId();

   UtlBoolean isCallId(const UtlString& sId);
   UtlBoolean isConferenceId(const UtlString& sId);
   UtlBoolean isSipCallId(const UtlString& sId);
   ID_TYPE getIdType(const UtlString& sId);

   static const int CALLMANAGER_MAX_REQUEST_MSGS;

   OsMutex m_memberMutex; ///< mutex for member synchronization, delete guard.
   // not thread safe fields
   UtlHashMap m_callMap; ///< hashmap with calls
   UtlHashMap m_conferenceMap; ///< hashmap with conferences

   // thread safe fields
   SipCallIdGenerator m_callIdGenerator; ///< generates string ids for calls
   SipCallIdGenerator m_conferenceIdGenerator; ///< generates string ids for conferences
   SipCallIdGenerator m_sipCallIdGenerator; ///< generates string sip call-ids

   UtlBoolean m_bDoNotDisturb; ///< if DND is enabled, we reject inbound calls (INVITE)
   UtlBoolean m_bEnableICE; 
   // read only fields
   const int m_rtpPortStart;
   const int m_rtpPortEnd;
};

#endif // XCallManager_h__
