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
template <class T>
class OsPtrLock; // forward template class declaration
class XCpAbstractCall;
class XCpCall;
class XCpConference;

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

   /**
   * Finds and returns a call or conference as XCpAbstractCall according to given id.
   * Returned OsPtrLock unlocks XCpAbstractCall automatically, and the object should not
   * be used outside its scope.
   * @param sID Identifier of call or conference. Must not be sip call-id.
   *
   * @return TRUE if a call or conference was found, FALSE otherwise.
   */
   UtlBoolean findAbstractCallById(const UtlString& sId, OsPtrLock<XCpAbstractCall>& ptrLock);

   /**
   * Finds and returns a call or conference as XCpAbstractCall according to given sip call-id.
   * Returned OsPtrLock unlocks XCpAbstractCall automatically, and the object should not
   * be used outside its scope.
   * @param sSipCallId Sip call-id of the call to find.
   * @param sLocalTag Tag of From SIP message field if known
   * @param sRemoteTag Tag of To SIP message field if known
   *
   * @return TRUE if a call or conference was found, FALSE otherwise.
   */
   UtlBoolean findAbstractCallBySipDialog(const UtlString& sSipCallId,
                                          const UtlString& sLocalTag,
                                          const UtlString& sRemoteTag,
                                          OsPtrLock<XCpAbstractCall>& ptrLock);

   /**
   * Finds and returns a XCpCall according to given id.
   * Returned OsPtrLock unlocks XCpCall automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a call was found, FALSE otherwise.
   */
   UtlBoolean findCall(const UtlString& sId, OsPtrLock<XCpCall>& ptrLock);

   /**
   * Finds and returns a XCpConference according to given id.
   * Returned OsPtrLock unlocks XCpConference automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a conference was found, FALSE otherwise.
   */
   UtlBoolean findConference(const UtlString& sId, OsPtrLock<XCpConference>& ptrLock);

   /**
    * Pushes given XCpCall on the call stack. Call must not be locked to avoid deadlocks.
    * Only push newly created calls.
    */
   UtlBoolean push(XCpCall& call);

   /**
    * Pushes given XCpCall on the conference stack. Conference must not be locked to avoid deadlocks.
    * Only push newly created conferences.
    */
   UtlBoolean push(XCpConference& conference);

   /**
    * Deletes call identified by Id from stack. Doesn't hang up the call, just shuts
    * media resources and deletes the call.
    */
   UtlBoolean deleteCall(const UtlString& sId);

   /**
    * Deletes conference identified by Id from stack. Doesn't hang up the conference, just shuts
    * media resources and deletes the conference.
    */
   UtlBoolean deleteConference(const UtlString& sId);

   /**
   * Deletes abstract call identified by Id from stack. Doesn't hang up the call, just shuts
   * media resources and deletes the call. Works for both calls and conferences.
   */
   UtlBoolean deleteAbstractCall(const UtlString& sId);

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
