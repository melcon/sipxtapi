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
#include <cp/CpDefs.h>
#include <cp/CpNatTraversalConfig.h>
#include <cp/XCpCallStack.h>
#include <cp/CpAudioCodecInfo.h>
#include <cp/CpVideoCodecInfo.h>

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
class OsNotification;
class UtlSList;
class SipDialog;
class SipUserAgent;
class SipLineProvider;
class SdpCodecList;
class XCpAbstractCall;
class XCpCall;
class XCpConference;
class CpMediaInterfaceFactory;
class CpCallStateEventListener;
class SipInfoStatusEventListener;
class SipSecurityEventListener;
class CpMediaEventListener;
class SipMessageEvent;
class SipMessage;
class ScCommandMsg;
class ScNotificationMsg;

/**
 * Call manager class. Responsible for creation of calls, management of calls via various operations, conferencing.
 *
 * maxCalls value is not strictly respected, it is only a soft limit that can be exceeded a little bit if there are
 * lots of inbound calls at the same time. Set it to number of calls that cause 80% of CPU consumption.
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
                  SipUserAgent& rSipUserAgent,
                  const SdpCodecList& rSdpCodecList,
                  SipLineProvider* pSipLineProvider,
                  const UtlString& sLocalIpAddress, // default IP address for CpMediaInterface
                  UtlBoolean doNotDisturb,
                  UtlBoolean bEnableICE,
                  UtlBoolean bEnableSipInfo,
                  UtlBoolean bIsRequiredLineMatch,
                  int rtpPortStart,
                  int rtpPortEnd,
                  int maxCalls, // max calls before sending busy. -1 means unlimited. Doesn't limit outbound calls.
                  int inviteExpireSeconds, // default use 180
                  CpMediaInterfaceFactory& rMediaInterfaceFactory);

   virtual ~XCpCallManager();

   /* ============================ MANIPULATORS ============================== */

   virtual UtlBoolean handleMessage(OsMsg& rRawMsg);

   virtual void requestShutdown(void);

   /** Creates new empty call. If sCallId, new id is generated. Generated id is returned in sCallId */
   OsStatus createCall(UtlString& sCallId);

   /** Creates new empty conference. If sConferenceId, new id is generated. Generated id is returned in sConferenceId */
   OsStatus createConference(UtlString& sConferenceId);

   /** 
    * Connects an existing call identified by id, to given address returning SipDialog.
    * SipDialog can be used to retrieve sip call-id and from tag
    */
   OsStatus connectCall(const UtlString& sCallId,
                        SipDialog& sSipDialog,
                        const UtlString& toAddress,
                        const UtlString& fullLineUrl,// includes display name, SIP URI
                        const UtlString& sSipCallId, // can be used to suggest sip call-id
                        const UtlString& locationHeader,
                        CP_CONTACT_ID contactId);

   /** 
    * Connects a call in an existing conference identified by id, to given address returning SipDialog.
    * SipDialog can be used to retrieve sip call-id and from tag
    */
   OsStatus connectConferenceCall(const UtlString& sConferenceId,
                                  SipDialog& sSipDialog,
                                  const UtlString& toAddress,
                                  const UtlString& fullLineUrl,// includes display name, SIP URI
                                  const UtlString& sSipCallId, // can be used to suggest sip call-id
                                  const UtlString& locationHeader,
                                  CP_CONTACT_ID contactId);

   /** 
    * Accepts inbound call connection. Inbound connections can only be part of XCpCall
    *
    * Progress the connection from the OFFERING state to the
    * RINGING state. This causes a SIP 180 Ringing provisional
    * response to be sent.
    */
   OsStatus acceptCallConnection(const UtlString& sCallId,
                                 const UtlString& locationHeader,
                                 CP_CONTACT_ID contactId);

   /**
    * Reject the incoming connection.
    *
    * Progress the connection from the OFFERING state to
    * the FAILED state with the cause of busy. With SIP this
    * causes a 486 Busy Here response to be sent.
    */
   OsStatus rejectCallConnection(const UtlString& sCallId);

   /**
    * Redirect the incoming connection.
    *
    * Progress the connection from the OFFERING state to
    * the FAILED state. This causes a SIP 302 Moved
    * Temporarily response to be sent with the specified
    * contact URI.
    */
   OsStatus redirectCallConnection(const UtlString& sCallId,
                                   const UtlString& sRedirectSipUrl);

   /**
    * Answer the incoming terminal connection.
    *
    * Progress the connection from the OFFERING or RINGING state
    * to the ESTABLISHED state and also creating the terminal
    * connection (with SIP a 200 OK response is sent).
    */
   OsStatus answerCallConnection(const UtlString& sCallId);

   /** 
    * Disconnects given simple call or conference call, not destroying the XCpCall/XCpConference
    * object so that it can be reused
    *
    * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
    * progresses to disconnected and the connection is removed.
    */
   OsStatus dropAbstractCallConnection(const UtlString& sAbstractCallId,
                                       const SipDialog& sSipDialog);

   /**
    * Disconnects all connections of abstract call (call, conference).
    *
    * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
    * progresses to disconnected and the connection is removed. The XCpAbstractCall object is not destroyed.
    */
   OsStatus dropAllAbstractCallConnections(const UtlString& sAbstractCallId);

   /** 
    * Disconnects given simple call not destroying the XCpCall object so that it can be reused.
    *
    * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
    * progresses to disconnected and the connection is removed. The XCpCall object is not destroyed.
    */
   OsStatus dropCallConnection(const UtlString& sCallId);

   /** 
   * Disconnects given conference call not destroying the XCpConference object so that it can be reused.
   *
   * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
   * progresses to disconnected and the connection is removed. The XCpConference object is not destroyed.
   */
   OsStatus dropConferenceConnection(const UtlString& sConferenceId,
                                     const SipDialog& sSipDialog);

   /**
    * Disconnects given all conference calls not destroying the XCpConference object so that it can be reused.
    *
    * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
    * progresses to disconnected and the connection is removed. The XCpConference object is not destroyed.
    */
   OsStatus dropAllConferenceConnections(const UtlString& sConferenceId);

   /**
   * Drops all XCpCall/XCpConference connections, and destroys the object once all connections
   * are disconnected. Asynchronous method.
   */
   OsStatus dropAbstractCall(const UtlString& sAbstractCallId);

   /**
   * Drops XCpCall connection, and destroys the object once connection
   * is disconnected. Asynchronous method.
   */
   OsStatus dropCall(const UtlString& sCallId);

   /**
   * Drops all XCpConference connections, and destroys the object once all connections
   * are disconnected. Asynchronous method.
   */
   OsStatus dropConference(const UtlString& sConferenceId);

   /** Deletes the XCpCall/XCpConference object, doesn't disconnect call */
   OsStatus destroyAbstractCall(const UtlString& sAbstractCallId);

   /** Deletes the XCpCall object, doesn't disconnect call */
   OsStatus destroyCall(const UtlString& sCallId);

   /** Deletes the XCpConference object, doesn't disconnect conference calls */
   OsStatus destroyConference(const UtlString& sConferenceId);

   /** Blind transfer given call to sTransferSipUri. Works for simple call and call in a conference */
   OsStatus transferBlindAbstractCall(const UtlString& sAbstractCallId,
                                      const SipDialog& sSipDialog,
                                      const UtlString& sTransferSipUrl);

   /**
    * Transfer an individual participant from one end point to another using REFER w/replaces.
    */
   OsStatus transferConsultativeAbstractCall(const UtlString& sSourceAbstractCallId,
                                             const SipDialog& sSourceSipDialog,
                                             const UtlString& sTargetAbstractCallId,
                                             const SipDialog& sTargetSipDialog);

   /** Starts DTMF tone on call connection.*/
   OsStatus audioToneStart(const UtlString& sAbstractCallId,
                           int iToneId,
                           UtlBoolean bLocal,
                           UtlBoolean bRemote);

   /** Stops DTMF tone on call connection */
   OsStatus audioToneStop(const UtlString& sAbstractCallId);

   /** Starts playing audio file on call connection */
   OsStatus audioFilePlay(const UtlString& sAbstractCallId,
                          const UtlString& audioFile,
                          UtlBoolean bRepeat,
                          UtlBoolean bLocal,
                          UtlBoolean bRemote,
                          UtlBoolean bMixWithMic = FALSE,
                          int iDownScaling = 100,
                          void* pCookie = NULL);

   /** Starts playing audio buffer on call connection. Passed buffer will be copied internally. */
   OsStatus audioBufferPlay(const UtlString& sAbstractCallId,
                            const void* pAudiobuf,
                            size_t iBufSize,
                            int iType,
                            UtlBoolean bRepeat,
                            UtlBoolean bLocal,
                            UtlBoolean bRemote,
                            UtlBoolean bMixWithMic = FALSE,
                            int iDownScaling = 100,
                            void* pCookie = NULL);

   /** Stops playing audio file or buffer on call connection */
   OsStatus audioStopPlayback(const UtlString& sAbstractCallId);

   /** Pauses audio playback of file or buffer. */
   OsStatus pauseAudioPlayback(const UtlString& sAbstractCallId);

   /** Resumes audio playback of file or buffer */
   OsStatus resumeAudioPlayback(const UtlString& sAbstractCallId);

   /** Starts recording call or conference. */
   OsStatus audioRecordStart(const UtlString& sAbstractCallId,
                             const UtlString& sFile);

   /** Stops recording call or conference. */
   OsStatus audioRecordStop(const UtlString& sAbstractCallId);

   /**
    * Put the specified terminal connection on hold. Works for
    * calls and conferences.
    *
    * Change the terminal connection state from TALKING to HELD.
    * (With SIP a re-INVITE message is sent with SDP indicating
    * no media should be sent.)
    */
   OsStatus holdAbstractCallConnection(const UtlString& sAbstractCallId,
                                       const SipDialog& sSipDialog);

   /**
   * Put the specified terminal connection on hold. Only works for calls.
   *
   * Change the terminal connection state from TALKING to HELD.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * no media should be sent.)
   */
   OsStatus holdCallConnection(const UtlString& sCallId);

   /**
    * Convenience method to put all of the terminal connections in
    * the specified conference on hold.
    */
   OsStatus holdAllConferenceConnections(const UtlString& sConferenceId);

   /**
    * Convenience method to put the local terminal connection on hold.
    * Can be used for both calls and conferences.
    * Microphone will be disconnected from the call or conference, local
    * audio will stop flowing to remote party. Remote party will still
    * be audible to local user.
    */
   OsStatus holdLocalAbstractCallConnection(const UtlString& sAbstractCallId);

   /**
    * Take the specified local terminal connection off hold,.
    *
    * Microphone will be reconnected to the call or conference,
    * and audio will start flowing from local machine to remote party.
    */
   OsStatus unholdLocalAbstractCallConnection(const UtlString& sAbstractCallId);

   /**
    * Convenience method to take all of the terminal connections in
    * the specified conference off hold.
    *
    * Change the terminal connection state from HELD to TALKING.
    * (With SIP a re-INVITE message is sent with SDP indicating
    * media should be sent.)
    */
   OsStatus unholdAllConferenceConnections(const UtlString& sConferenceId);

   /**
    * Convenience method to take the terminal connection off hold.
    * Works for both calls and conferences.
    *
    * Change the terminal connection state from HELD to TALKING.
    * (With SIP a re-INVITE message is sent with SDP indicating
    * media should be sent.)
    */
   OsStatus unholdAbstractCallConnection(const UtlString& sAbstractCallId,
                                         const SipDialog& sSipDialog);

   /**
   * Convenience method to take the terminal connection off hold.
   * Works only for calls.
   *
   * Change the terminal connection state from HELD to TALKING.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * media should be sent.)
   */
   OsStatus unholdCallConnection(const UtlString& sCallId);

   /**
    * Enables discarding of inbound RTP at bridge for given call
    * or conference. Useful for server applications without mic/speaker.
    * DTMF on given call will still be decoded.
    */
   OsStatus muteInputAbstractCallConnection(const UtlString& sAbstractCallId,
                                            const SipDialog& sSipDialog);

   /**
   * Disables discarding of inbound RTP for given call
   * or conference. Useful for server applications without mic/speaker.
   */
   OsStatus unmuteInputAbstractCallConnection(const UtlString& sAbstractCallId,
                                              const SipDialog& sSipDialog);

   /**
    * Rebuild codec factory of the call (media interface) on the fly with new audio
    * codec requirements and new video codecs. Preferences will be in effect after the next
    * INVITE or re-INVITE. Can be called on empty call or conference to limit
    * codecs for future calls. When called on an established call, hold/unhold
    * or codec renegotiation needs to be triggered to actually change codecs.
    * If used on conference, codecs will be applied to all future calls, and all
    * calls that are unheld.
    *
    * This method doesn't affect codec factory used for new outbound/inbound calls.
    */
   OsStatus limitAbstractCallCodecPreferences(const UtlString& sAbstractCallId,
                                              const UtlString& sAudioCodecs,
                                              const UtlString& sVideoCodecs);

   /**
    * Rebuild codec factory on the fly with new audio codec requirements
    * and specified video codecs.  Renegotiate the codecs to be use for the
    * specified terminal connection. For call overrides preferences set by
    * limitAbstractCallCodecPreferences. For conference, only overrides
    * codecs used by given call. Conference call will then ignore preferences set
    * by previous call to limitAbstractCallCodecPreferences when unhold is executed.
    * But next call of limitAbstractCallCodecPreferences will again override this setting.
    *
    * This is typically performed after a capabilities change for the
    * terminal connection (for example, addition or removal of a codec type).
    * (Sends a SIP re-INVITE.)
    */
   OsStatus renegotiateCodecsAbstractCallConnection(const UtlString& sAbstractCallId,
                                                    const SipDialog& sSipDialog,
                                                    const UtlString& sAudioCodecs,
                                                    const UtlString& sVideoCodecs);

   /**
   * Rebuild codec factory on the fly with new audio codec requirements
   * and specified video codecs. Convenience method to renegotiate the codecs
   * for all of the terminal connections in the specified conference.
   *
   * This is typically performed after a capabilities change for the
   * terminal connection (for example, addition or removal of a codec type).
   * (Sends a SIP re-INVITE.)
   */
   OsStatus renegotiateCodecsAllConferenceConnections(const UtlString& sConferenceId,
                                                      const UtlString& sAudioCodecs,
                                                      const UtlString& sVideoCodecs);

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

   /** Sends an INFO message to the other party(s) on the call. Allows transfer of binary data. */
   OsStatus sendInfo(const UtlString& sAbstractCallId,
                     const SipDialog& sSipDialog,
                     const UtlString& sContentType,
                     const char* pContent,
                     const size_t nContentLength);

   /** Generates new sip call-id */
   UtlString getNewSipCallId();

   /** Generates new id for call. It is not the call-id field used in sip messages, instead its an internal id */
   UtlString getNewCallId();

   /** Generates new id for conference */
   UtlString getNewConferenceId();

   /* ============================ ACCESSORS ================================= */

   UtlBoolean getDoNotDisturb() const { return m_bDoNotDisturb; }
   void setDoNotDisturb(UtlBoolean val) { m_bDoNotDisturb = val; }

   UtlBoolean getEnableICE() const { return m_natTraversalConfig.m_bEnableICE; }
   void setEnableICE(UtlBoolean val) { m_natTraversalConfig.m_bEnableICE = val; }

   /** Enable/disable reception of SIP INFO. Sending is always allowed. Only affects new calls. */
   UtlBoolean getEnableSipInfo() const { return m_bEnableSipInfo; }
   void setEnableSipInfo(UtlBoolean val) { m_bEnableSipInfo = val; }

   int getMaxCalls() const { return m_maxCalls; }
   void setMaxCalls(int val) { m_maxCalls = val; }

   CpMediaInterfaceFactory* getMediaInterfaceFactory() const;

   /* ============================ INQUIRY =================================== */

   /** gets total amount of calls. Also calls in conference are counted */
   int getCallCount() const;

   /** Gets ids of all calls and conferences. Ids are appended into list. */
   OsStatus getAbstractCallIds(UtlSList& idList) const;

   /** Gets ids of all calls. Ids are appended into list. */
   OsStatus getCallIds(UtlSList& callIdList) const;

   /** Gets ids of all conferences. Ids are appended into list. */
   OsStatus getConferenceIds(UtlSList& conferenceIdList) const;

   /** Gets sip call-id of call if its available */
   OsStatus getCallSipCallId(const UtlString& sCallId,
                             UtlString& sSipCallId) const;

   /** Gets sip call-ids of conference if available */
   OsStatus getConferenceSipCallIds(const UtlString& sConferenceId,
                                    UtlSList& sipCallIdList) const;

   /** Gets remote user agent for call or conference */
   OsStatus getRemoteUserAgent(const UtlString& sAbstractCallId,
                               const SipDialog& sSipDialog,
                               UtlString& userAgent) const;

   /** Gets internal id of media connection for given call or conference. Only for unit tests */
   OsStatus getMediaConnectionId(const UtlString& sAbstractCallId,
                                 const SipDialog& sSipDialog,
                                 int& mediaConnID) const;

   /** 
    * Gets copy of SipDialog for given call. This SipDialog will have all available information in it,
    * not just sip call-id and tags.
    */
   OsStatus getSipDialog(const UtlString& sAbstractCallId,
                         const SipDialog& sSipDialog,
                         SipDialog& sOutputSipDialog) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XCpCallManager(const XCpCallManager& rhs);

   XCpCallManager& operator=(const XCpCallManager& rhs);

   /** Checks if we can create new call. Only used when new inbound call is created. */
   UtlBoolean checkCallLimit();

   /** Handler for OsMsg::PHONE_APP messages */
   UtlBoolean handlePhoneAppMessage(const OsMsg& rRawMsg);

   /** Handler for CpMessageTypes::SC_COMMAND messages */
   UtlBoolean handleSipConnectionCommandMessage(const ScCommandMsg& rMsg);

   /** Handler for CpMessageTypes::ScNotificationMsg messages */
   UtlBoolean handleSipConnectionNotificationMessage(const ScNotificationMsg& rMsg);

   /** Handler for inbound SipMessageEvent messages. Tries to find call for event. */
   UtlBoolean handleSipMessageEvent(const SipMessageEvent& rSipMsgEvent);

   /** Handles some inbound SipMessageEvent for which call wasn't found */
   UtlBoolean handleUnknownSipMessageEvent(const SipMessageEvent& rSipMsgEvent);

   /** Handler for inbound INVITE SipMessage, for which there is no existing call or conference. */
   UtlBoolean handleUnknownInviteRequest(const SipMessage& rSipMessage);

   /** Handler for inbound UPDATE SipMessage, for which there is no existing call or conference. */
   UtlBoolean handleUnknownUpdateRequest(const SipMessage& rSipMessage);

   /** Handler for inbound OPTIONS SipMessage, for which there is no existing call or conference. */
   UtlBoolean handleUnknownOptionsRequest(const SipMessage& rSipMessage);

   /** Handler for inbound REFER SipMessage, for which there is no existing call or conference. */
   UtlBoolean handleUnknownReferRequest(const SipMessage& rSipMessage);

   /** Handler for inbound CANCEL SipMessage, for which there is no existing call or conference. */
   UtlBoolean handleUnknownCancelRequest(const SipMessage& rSipMessage);

   /** Handler for inbound PRACK SipMessage, for which there is no existing call or conference. */
   UtlBoolean handleUnknownPrackRequest(const SipMessage& rSipMessage);

   /** Does some basic checks on the SipMessage itself, and sends response if message is rejected. */
   UtlBoolean handleUnknownSipRequest(const SipMessage& rSipMessage);

   /** Creates new XCpCall, starts it and posts message into it for handling. */
   void createNewInboundCall(const SipMessage& rSipMessage);

   /** Sends 481 Call/Transaction Does Not Exist as reply to given message */
   UtlBoolean sendBadTransactionError(const SipMessage& rSipMessage);

   /** Initializes SipMessage observation on m_rSipUserAgent */
   void startSipMessageObserving();

   /** Stops observing SipMessages */
   void stopSipMessageObserving();

   static const int CALLMANAGER_MAX_REQUEST_MSGS;

   mutable OsMutex m_memberMutex; ///< mutex for member synchronization, delete guard.

   // not thread safe fields
   CpNatTraversalConfig m_natTraversalConfig; ///< configuration of NAT traversal options

   // thread safe fields
   XCpCallStack m_callStack; ///< call stack for storing XCpCall and XCpConference instances
   SipCallIdGenerator m_sipCallIdGenerator; ///< generates string sip call-ids

   CpMediaInterfaceFactory& m_rMediaInterfaceFactory;
   CpCallStateEventListener* m_pCallEventListener; // listener for firing call events
   SipInfoStatusEventListener* m_pInfoStatusEventListener; // listener for firing info events
   SipSecurityEventListener* m_pSecurityEventListener; // listener for firing security events
   CpMediaEventListener* m_pMediaEventListener; // listener for firing media events
   SipUserAgent& m_rSipUserAgent; // sends sip messages
   const SdpCodecList& m_rDefaultSdpCodecList; ///< list for SDP codecs supplied to constructor
   SipLineProvider* m_pSipLineProvider; // read only functionality of line manager

   // thread safe atomic
   UtlBoolean m_bDoNotDisturb; ///< if DND is enabled, we reject inbound calls (INVITE)
   UtlBoolean m_bEnableSipInfo; ///< whether INFO support is enabled for new calls. If disabled, we send "415 Unsupported Media Type"
   UtlBoolean m_bIsRequiredLineMatch; ///< if inbound SIP message must correspond to some line to be handled
   int m_maxCalls; ///< maximum number of calls we should support. -1 means unlimited. In effect only when new inbound call arrives.
   int m_inviteExpireSeconds;

   // read only fields
   const int m_rtpPortStart;
   const int m_rtpPortEnd;
   const UtlString m_sLocalIpAddress;
};

#endif // XCallManager_h__
