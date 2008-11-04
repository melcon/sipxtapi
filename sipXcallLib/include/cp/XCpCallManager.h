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
class SdpCodecFactory;
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
                  SdpCodecFactory& rSdpCodecFactory,
                  SipLineProvider* pSipLineProvider,
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
                            void* pCookie = NULL);

   /** Stops playing audio file or buffer on call connection */
   OsStatus audioStop(const UtlString& sAbstractCallId);

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
    * Rebuild codec factory on the fly with new audio codec requirements
    * and new video codecs. Preferences will be in effect after the next
    * INVITE or re-INVITE. Can be called on empty call or conference to limit
    * codecs for future calls. When called on an established call, hold/unhold
    * or codec renegotiation needs to be triggered to actually change codecs.
    * If used on conference, codecs will be applied to all future calls, and all
    * calls that are unheld.
    */
   OsStatus limitAbstractCallCodecPreferences(const UtlString& sAbstractCallId,
                                              CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                              const UtlString& sAudioCodecs,
                                              CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
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
                                                    CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                    const UtlString& sAudioCodecs,
                                                    CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
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
                                                      CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                      const UtlString& sAudioCodecs,
                                                      CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
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

   typedef enum
   {
      ID_TYPE_CALL,
      ID_TYPE_CONFERENCE,
      ID_TYPE_UNKNOWN
   } ID_TYPE;

   /** Checks if given Id identifies a call instance */
   static UtlBoolean isCallId(const UtlString& sId);

   /** Checks if given Id identifies a conference instance */
   static UtlBoolean isConferenceId(const UtlString& sId);

   /** Gets the type of Id. Can be call, conference or unknown. */
   static ID_TYPE getIdType(const UtlString& sId);

   /**
   * Finds and returns a call or conference as XCpAbstractCall according to given id.
   * Returned OsPtrLock unlocks XCpAbstractCall automatically, and the object should not
   * be used outside its scope.
   * @param sID Identifier of call or conference. Must not be sip call-id.
   *
   * @return TRUE if a call or conference was found, FALSE otherwise.
   */
   UtlBoolean findAbstractCall(const UtlString& sAbstractCallId,
                               OsPtrLock<XCpAbstractCall>& ptrLock) const;

   /**
   * Gets some abstract call, which is different from supplied id. Useful when some call
   * is getting shut down, and some resource needs to move to other call, different from
   * the old one.
   * @param sID Identifier of call or conference to avoid. Must not be sip call-id.
   *
   * @return TRUE if a call or conference was found, FALSE otherwise.
   */
   UtlBoolean findSomeAbstractCall(const UtlString& sAvoidAbstractCallId,
                                   OsPtrLock<XCpAbstractCall>& ptrLock) const;

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
   UtlBoolean findAbstractCall(const SipDialog& sSipDialog,
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
   * Finds and returns a XCpCall according to given SipDialog.
   * Returned OsPtrLock unlocks XCpCall automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a call was found, FALSE otherwise.
   */
   UtlBoolean findCall(const SipDialog& sSipDialog, OsPtrLock<XCpCall>& ptrLock) const;

   /**
   * Finds and returns a XCpConference according to given id.
   * Returned OsPtrLock unlocks XCpConference automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a conference was found, FALSE otherwise.
   */
   UtlBoolean findConference(const UtlString& sId, OsPtrLock<XCpConference>& ptrLock) const;

   /**
   * Finds and returns a XCpConference according to given SipDialog.
   * Returned OsPtrLock unlocks XCpConference automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a conference was found, FALSE otherwise.
   */
   UtlBoolean findConference(const SipDialog& sSipDialog, OsPtrLock<XCpConference>& ptrLock) const;

   /**
    * Finds and returns XCpAbstractCall capable of handling given SipMessage.
    *
    * @return TRUE if an XCpAbstractCall was found, FALSE otherwise.
    */
   UtlBoolean findHandlingAbstractCall(const SipMessage& rSipMessage, OsPtrLock<XCpAbstractCall>& ptrLock) const;

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
   UtlBoolean deleteCall(const UtlString& sCallId);

   /**
    * Deletes conference identified by Id from stack. Doesn't hang up the conference, just shuts
    * media resources and deletes the conference.
    */
   UtlBoolean deleteConference(const UtlString& sConferenceId);

   /**
   * Deletes abstract call identified by Id from stack. Doesn't hang up the call, just shuts
   * media resources and deletes the call. Works for both calls and conferences.
   */
   UtlBoolean deleteAbstractCall(const UtlString& sAbstractCallId);

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

   /** Checks if we can create new call. Only used when new inbound call is created. */
   UtlBoolean checkCallLimit();

   /** Handler for OsMsg::PHONE_APP messages */
   UtlBoolean handlePhoneAppMessage(const OsMsg& rRawMsg);

   /** Handler for inbound SipMessageEvent messages. Tries to find call for event. */
   UtlBoolean handleSipMessageEvent(const SipMessageEvent& rSipMsgEvent);

   /** Handler for inbound SipMessageEvent, for which there is no existing call or conference. */
   UtlBoolean handleUnknownSipMessageEvent(const SipMessageEvent& rSipMsgEvent);

   /** Checks if we should further process SipMessage. Checks are done on the event. Sends response if rejected. */
   UtlBoolean basicSipMessageEventCheck(const SipMessageEvent& rSipMsgEvent);

   /** Does some basic checks on the SipMessage itself, and sends response if message is rejected. */
   UtlBoolean basicSipMessageRequestCheck(const SipMessage& rSipMessage);

   /** Creates new XCpCall, starts it and posts message into it for handling. */
   void createNewInboundCall(const SipMessageEvent& rSipMsgEvent);

   /** Gains focus for given call, defocusing old focused call. */
   OsStatus doGainFocus(const UtlString& sAbstractCallId,
                        UtlBoolean bGainOnlyIfNoFocusedCall = FALSE);

   /**
   * Gains focus for next call, avoiding sAvoidAbstractCallId when looking for next call to focus.
   * If there is no other call than sAvoidAbstractCallId, then no focus is gained. Meant to be used
   * from doYieldFocus to gain next focus. Works only if no call has currently focus.
   */
   OsStatus doGainNextFocus(const UtlString& sAvoidAbstractCallId);

   /**
   * Defocuses given call if its focused. Shifts focus to next call if requested.
   * Has no effect if given call is not focused anymore.
   */
   OsStatus doYieldFocus(const UtlString& sAbstractCallId,
                         UtlBoolean bShiftFocus = TRUE);

   /** Defocuses current call in focus, and lets other call gain focus if requested */
   OsStatus doYieldFocus(UtlBoolean bShiftFocus = TRUE);

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

   // focus
   mutable OsMutex m_focusMutex; ///< required for access to m_sAbstractCallInFocus
   UtlString m_sAbstractCallInFocus; ///< holds id of call currently in focus.

   // thread safe fields
   SipCallIdGenerator m_callIdGenerator; ///< generates string ids for calls
   SipCallIdGenerator m_conferenceIdGenerator; ///< generates string ids for conferences
   SipCallIdGenerator m_sipCallIdGenerator; ///< generates string sip call-ids

   CpMediaInterfaceFactory& m_rMediaInterfaceFactory;
   CpCallStateEventListener* m_pCallEventListener; // listener for firing call events
   SipInfoStatusEventListener* m_pInfoStatusEventListener; // listener for firing info events
   SipSecurityEventListener* m_pSecurityEventListener; // listener for firing security events
   CpMediaEventListener* m_pMediaEventListener; // listener for firing media events
   SipUserAgent& m_rSipUserAgent; // sends sip messages
   SdpCodecFactory& m_rSdpCodecFactory;
   SipLineProvider* m_pSipLineProvider; // read only functionality of line manager

   // thread safe atomic
   UtlBoolean m_bDoNotDisturb; ///< if DND is enabled, we reject inbound calls (INVITE)
   UtlBoolean m_bEnableICE; 
   UtlBoolean m_bEnableSipInfo; ///< whether INFO support is enabled for new calls. If disabled, we send "415 Unsupported Media Type"
   UtlBoolean m_bIsRequiredLineMatch; ///< if inbound SIP message must correspond to some line to be handled
   int m_maxCalls; ///< maximum number of calls we should support. -1 means unlimited. In effect only when new inbound call arrives.
   int m_inviteExpireSeconds;

   // read only fields
   const int m_rtpPortStart;
   const int m_rtpPortEnd;

};

#endif // XCallManager_h__
