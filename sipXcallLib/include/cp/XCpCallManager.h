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
class CpMediaInterfaceFactory;
class UtlSList;
class SipUserAgent;
class CpCallStateEventListener;
class SipInfoStatusEventListener;
class SipSecurityEventListener;
class CpMediaEventListener;
class SipDialog;

/**
 * Call manager class. Responsible for creation of calls, management of calls via various operations, conferencing.
 */
class XCpCallManager : public OsServerTask
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   XCpCallManager(CpCallStateEventListener* pCallEventListener,
                  SipInfoStatusEventListener* pInfoStatusEventListener,
                  SipSecurityEventListener* pSecurityEventListener,
                  CpMediaEventListener* pMediaEventListener,
                  SipUserAgent* pSipUserAgent,
                  UtlBoolean doNotDisturb,
                  UtlBoolean bEnableICE,
                  UtlBoolean bEnableSipInfo,
                  int rtpPortStart,
                  int rtpPortEnd,
                  int maxCalls, // max calls before sending busy. -1 means unlimited
                  CpMediaInterfaceFactory* pMediaFactory);

   virtual ~XCpCallManager();

   /* ============================ MANIPULATORS ============================== */

   virtual UtlBoolean handleMessage(OsMsg& rRawMsg);

   virtual void requestShutdown(void);

   /** Creates new empty call, returning id if successful */
   OsStatus createCall(UtlString& id);

   /** Creates new empty conference, returning id if successful */
   OsStatus createConference(UtlString& id);

   /** Enable STUN for NAT/Firewall traversal */
   void enableStun(const UtlString& sStunServer, 
                   int iServerPort,
                   int iKeepAlivePeriodSecs = 0, 
                   OsNotification* pNotification = NULL);

   /** Enable TURN for NAT/Firewall traversal */
   void enableTurn(const UtlString& sTurnServer,
                   int iTurnPort,
                   const UtlString& sTurnUsername,
                   const UtlString& sTurnPassword,
                   int iKeepAlivePeriodSecs = 0);

   /** Sends an INFO message to the other party(s) on the call */
   OsStatus sendInfo(const UtlString& sId,
                     const UtlString& sSipCallId,
                     const UtlString& sLocalTag,
                     const UtlString& sRemoteTag,
                     const UtlString& sContentType,
                     const UtlString& sContentEncoding,
                     const UtlString& sContent);

   /* ============================ ACCESSORS ================================= */

   UtlBoolean getDoNotDisturb() const { return m_bDoNotDisturb; }
   void setDoNotDisturb(UtlBoolean val) { m_bDoNotDisturb = val; }

   UtlBoolean getEnableICE() const { return m_bEnableICE; }
   void setEnableICE(UtlBoolean val) { m_bEnableICE = val; }

   /** Enable/disable reception of SIP INFO. Sending is always allowed. Only affects new calls. */
   UtlBoolean getEnableSipInfo() const { return m_bEnableSipInfo; }
   void setEnableSipInfo(UtlBoolean val) { m_bEnableSipInfo = val; }

   int getMaxCalls() const { return m_maxCalls; }
   void setMaxCalls(int val) { m_maxCalls = val; }

   CpMediaInterfaceFactory* getMediaInterfaceFactory() const;

   /* ============================ INQUIRY =================================== */

   /** gets total amount of calls. Also calls in conference are counted */
   int getCallCount() const;

   /** Gets ids of all calls */
   OsStatus getCallIds(UtlSList& idList) const;

   /** Gets ids of all conferences */
   OsStatus getConferenceIds(UtlSList& idList) const;

   /** Gets audio energy levels for call or conference identified by sId */
   OsStatus getAudioEnergyLevels(const UtlString& sId,
                                 int& iInputEnergyLevel,
                                 int& iOutputEnergyLevel) const;

   /** Gets remote user agent for call or conference */
   OsStatus getRemoteUserAgent(const UtlString& sId,
                               const UtlString& sSipCallId,
                               const UtlString& sLocalTag,
                               const UtlString& sRemoteTag,
                               UtlString& userAgent) const;

   /** Gets internal id of media connection for given call or conference. Only for unit tests */
   OsStatus getMediaConnectionId(const UtlString& sId, int& mediaConnID) const;

   /** Gets copy of SipDialog for given call */
   OsStatus getSipDialog(const UtlString& sId,
                         const UtlString& sSipCallId,
                         const UtlString& sLocalTag,
                         const UtlString& sRemoteTag,
                         SipDialog& dialog) const;

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
      ID_TYPE_UNKNOWN
   } ID_TYPE;

   /** Generates new id for call. It is not the call-id field used in sip messages, instead its an internal id */
   UtlString getNewCallId();

   /** Generates new id for conference */
   UtlString getNewConferenceId();

   /** Generates new sip call-id */
   UtlString getNewSipCallId();

   /** Checks if given Id identifies a call instance */
   UtlBoolean isCallId(const UtlString& sId) const;

   /** Checks if given Id identifies a conference instance */
   UtlBoolean isConferenceId(const UtlString& sId) const;

   /** Gets the type of Id. Can be call, conference or unknown. */
   ID_TYPE getIdType(const UtlString& sId) const;

   /**
   * Finds and returns a call or conference as XCpAbstractCall according to given id.
   * Returned OsPtrLock unlocks XCpAbstractCall automatically, and the object should not
   * be used outside its scope.
   * @param sID Identifier of call or conference. Must not be sip call-id.
   *
   * @return TRUE if a call or conference was found, FALSE otherwise.
   */
   UtlBoolean findAbstractCallById(const UtlString& sId, OsPtrLock<XCpAbstractCall>& ptrLock) const;

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
                                          OsPtrLock<XCpAbstractCall>& ptrLock) const;

   /**
   * Finds and returns a XCpCall according to given id.
   * Returned OsPtrLock unlocks XCpCall automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a call was found, FALSE otherwise.
   */
   UtlBoolean findCall(const UtlString& sId, OsPtrLock<XCpCall>& ptrLock) const;

   /**
   * Finds and returns a XCpConference according to given id.
   * Returned OsPtrLock unlocks XCpConference automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a conference was found, FALSE otherwise.
   */
   UtlBoolean findConference(const UtlString& sId, OsPtrLock<XCpConference>& ptrLock) const;

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

   /**
    * Deletes all calls on the stack, freeing any call resources. Doesn't properly terminate
    * the calls.
    */
   void deleteAllCalls();

   /**
    * Deletes all conferences on the stack, freeing any call resources. Doesn't properly terminate
    * the conferences.
    */
   void deleteAllConferences();

   /** Checks if we can create new call */
   UtlBoolean canCreateNewCall();

   static const int CALLMANAGER_MAX_REQUEST_MSGS;

   mutable OsMutex m_memberMutex; ///< mutex for member synchronization, delete guard.

   // not thread safe fields
   UtlHashMap m_callMap; ///< hashmap with calls
   UtlHashMap m_conferenceMap; ///< hashmap with conferences

   UtlString m_sStunServer; ///< address or ip of stun server
   int m_iStunPort; ///< port for stun server
   int m_iStunKeepAlivePeriodSecs; ///< stun refresh period

   UtlString m_sTurnServer; ///< turn server address or ip
   int m_iTurnPort; ///< turn server port
   UtlString m_sTurnUsername; ///< turn username
   UtlString m_sTurnPassword; ///< turn password
   int m_iTurnKeepAlivePeriodSecs; ///< turn refresh period

   // thread safe fields
   SipCallIdGenerator m_callIdGenerator; ///< generates string ids for calls
   SipCallIdGenerator m_conferenceIdGenerator; ///< generates string ids for conferences
   SipCallIdGenerator m_sipCallIdGenerator; ///< generates string sip call-ids

   CpMediaInterfaceFactory* m_pMediaFactory;
   CpCallStateEventListener* m_pCallEventListener;
   SipInfoStatusEventListener* m_pInfoStatusEventListener;
   SipSecurityEventListener* m_pSecurityEventListener;
   CpMediaEventListener* m_pMediaEventListener;
   SipUserAgent* m_pSipUserAgent;

   // thread safe atomic
   UtlBoolean m_bDoNotDisturb; ///< if DND is enabled, we reject inbound calls (INVITE)
   UtlBoolean m_bEnableICE; 
   UtlBoolean m_bEnableSipInfo; ///< whether INFO support is enabled for new calls. If disabled, we send "415 Unsupported Media Type"
   int m_maxCalls; ///< maximum number of calls we should support. -1 means unlimited

   // read only fields
   const int m_rtpPortStart;
   const int m_rtpPortEnd;
};

#endif // XCallManager_h__
