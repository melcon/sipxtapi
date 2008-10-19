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
                  int maxCalls, // max calls before sending busy. -1 means unlimited. Doesn't limit outbound calls.
                  CpMediaInterfaceFactory* pMediaFactory);

   virtual ~XCpCallManager();

   /* ============================ MANIPULATORS ============================== */

   virtual UtlBoolean handleMessage(OsMsg& rRawMsg);

   virtual void requestShutdown(void);

   /** Creates new empty call, returning id if successful */
   OsStatus createCall(UtlString& sCallId);

   /** Creates new empty conference, returning id if successful */
   OsStatus createConference(UtlString& sConferenceId);

   /** 
    * Connects an existing call identified by id, to given address returning SipDialog.
    * SipDialog can be used to retrieve sip call-id and from tag
    */
   OsStatus connectCall(const UtlString& sCallId,
                        SipDialog& sSipDialog,
                        const UtlString& toAddress,
                        const UtlString& lineURI,
                        const UtlString& locationHeader,
                        CP_CONTACT_ID contactId);

   /** 
    * Connects a call in an existing conference identified by id, to given address returning SipDialog.
    * SipDialog can be used to retrieve sip call-id and from tag
    */
   OsStatus connectConferenceCall(const UtlString& sConferenceId,
                                  SipDialog& sSipDialog,
                                  const UtlString& toAddress,
                                  const UtlString& lineURI,
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
                                   const UtlString& sRedirectSipUri);

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
    * Disconnects given simple call not destroying the XCpCall object so that it can be reused.
    *
    * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
    * progresses to disconnected and the connection is removed.
    */
   OsStatus dropCallConnection(const UtlString& sCallId);

   /**
    * Disconnects given all conference calls not destroying the XCpConference object so that it can be reused.
    *
    * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
    * progresses to disconnected and the connection is removed.
    */
   OsStatus dropAllConferenceConnections(const UtlString& sConferenceId);

   /** Deletes the XCpCall/XCpConference object, doesn't disconnect call */
   OsStatus dropAbstractCall(const UtlString& sAbstractCallId);

   /** Deletes the XCpCall object, doesn't disconnect call */
   OsStatus dropCall(const UtlString& sCallId);

   /** Deletes the XCpConference object, doesn't disconnect conference calls */
   OsStatus dropConference(const UtlString& sConferenceId);

   /** Blind transfer given call to sTransferSipUri. Works for simple call and call in a conference */
   OsStatus transferBlindAbstractCall(const UtlString& sAbstractCallId,
                                      const SipDialog& sSipDialog,
                                      const UtlString& sTransferSipUri);

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
                            void* pAudiobuf,
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
    * Enables discarding of inbound RTP for given call
    * or conference. Useful for server applications without mic/speaker.
    */
   OsStatus silentHoldRemoteAbstractCallConnection(const UtlString& sAbstractCallId,
                                                   const SipDialog& sSipDialog);

   /**
   * Disables discarding of inbound RTP for given call
   * or conference. Useful for server applications without mic/speaker.
   */
   OsStatus silentUnholdRemoteAbstractCallConnection(const UtlString& sAbstractCallId,
                                                     const SipDialog& sSipDialog);

   /**
   * Stops outbound RTP for given call or conference.
   * Useful for server applications without mic/speaker.
   */
   OsStatus silentHoldLocalAbstractCallConnection(const UtlString& sAbstractCallId,
                                                  const SipDialog& sSipDialog);

   /**
   * Starts outbound RTP for given call or conference.
   * Useful for server applications without mic/speaker.
   */
   OsStatus silentUnholdLocalAbstractCallConnection(const UtlString& sAbstractCallId,
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

   /** Sends an INFO message to the other party(s) on the call */
   OsStatus sendInfo(const UtlString& sAbstractCallId,
                     const SipDialog& sSipDialog,
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

   /** Gets sip call-id of call if its available */
   OsStatus getCallSipCallId(const UtlString& sCallId,
                             UtlString& sSipCallId) const;

   /** Gets sip call-ids of conference if available */
   OsStatus getConferenceSipCallIds(const UtlString& sConferenceId,
                                    UtlSList& sipCallIdList) const;

   /** Gets audio energy levels for call or conference identified by sId */
   OsStatus getAudioEnergyLevels(const UtlString& sAbstractCallId,
                                 int& iInputEnergyLevel,
                                 int& iOutputEnergyLevel) const;

   /** Gets remote user agent for call or conference */
   OsStatus getRemoteUserAgent(const UtlString& sAbstractCallId,
                               const SipDialog& sSipDialog,
                               UtlString& userAgent) const;

   /** Gets internal id of media connection for given call or conference. Only for unit tests */
   OsStatus getMediaConnectionId(const UtlString& sAbstractCallId,
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
   UtlBoolean findAbstractCall(const UtlString& sAbstractCallId,
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
   int m_maxCalls; ///< maximum number of calls we should support. -1 means unlimited. In effect only when new inbound call arrives.

   // read only fields
   const int m_rtpPortStart;
   const int m_rtpPortEnd;
};

#endif // XCallManager_h__
